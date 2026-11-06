#include "S47RoomDecor.h"
#include "S47Album.h"
#include "S47Freeform.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"

namespace
{
 // Above the corner PC, clear of the file cabinets and the room's corner.
 constexpr double SlotZ[FS47RoomDecor::SlotCount]={.85,1.75,2.65,3.55};
 constexpr double SlotY=1.82,FrameW=.76,FrameH=.58,PictureW=.66,PictureH=.48,FallbackWall=-6.2;
 FVector Size(double X,double Y,double Z){const FVector V=S47Three::DirToUE(FVector(X,Y,Z));return FVector(FMath::Abs(V.X),FMath::Abs(V.Y),FMath::Abs(V.Z));}
}

bool FS47RoomDecor::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web))return false;
 const FS47Obj Room=M->Web->Obj(TEXT("room.group"));if(!Room)return false;
 const auto* Node=M->Web->Node(Room.I);if(!Node||!Node->Comp.IsValid())return false;
 auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));if(!Cube||!Base)return false;
 // The wall's real face, so a frame neither floats nor sinks into it.
 double Wall=FallbackWall;
 FHitResult Hit;FCollisionQueryParams Query(TEXT("S47RoomDecor"),true);
 if(M->Web->GetWorld()&&M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(FVector(-3,SlotY,SlotZ[1])),S47Three::ToUE(FVector(-8,SlotY,SlotZ[1])),ECC_Visibility,Query))Wall=S47Three::ToThree(Hit.ImpactPoint).X;
 UE_LOG(LogTemp,Display,TEXT("S47: room decor wall at x=%.3f"),Wall);
 auto Piece=[&](const FString& Name,double X,double Z,const FVector& Scale,const FLinearColor& Color,UMaterialInstanceDynamic*& OutMat)
 {
  OutMat=UMaterialInstanceDynamic::Create(Base,M->Web);OutMat->SetVectorParameterValue(TEXT("Color"),Color);OutMat->SetScalarParameterValue(TEXT("Roughness"),.7);OutMat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  auto* C=NewObject<UStaticMeshComponent>(M->Web,*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(Node->Comp.Get());C->SetStaticMesh(Cube);C->SetMaterial(0,OutMat);
  C->SetWorldLocation(S47Three::ToUE(FVector(X,SlotY,Z)));C->SetWorldScale3D(Scale);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();
  return C;
 };
 for(int32 I=0;I<SlotCount;++I)
 {
  FFrame F;F.Id=FString::Printf(TEXT("ff:decor:%d"),I);
  UMaterialInstanceDynamic* BorderMat=nullptr;UMaterialInstanceDynamic* PictureMat=nullptr;
  auto* Border=Piece(FString::Printf(TEXT("S47DecorFrame%d"),I),Wall+.015,SlotZ[I],Size(.03,FrameH,FrameW),FLinearColor(.055,.035,.02),BorderMat);
  auto* Picture=Piece(FString::Printf(TEXT("S47DecorPicture%d"),I),Wall+.032,SlotZ[I],Size(.006,PictureH,PictureW),FLinearColor(.72,.68,.58),PictureMat);
  Border->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Border->SetCollisionResponseToAllChannels(ECR_Ignore);Border->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Border->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
  F.Border=Border;F.Picture=Picture;F.Material=PictureMat;
  F.Object=M->Web->ObjAt(M->Web->AdoptNode(Border,Room.I,F.Id));
  Frames.Add(F);
 }
 bBuilt=true;return true;
}

void FS47RoomDecor::Bind()
{
 if(!M||!IsValid(M->Inter))return;
 const TWeakPtr<uint8> Guard=Life;
 for(const FFrame& F:Frames)
 {
  const FString Id=F.Id;FS47Interactable I;I.Id=Id;I.Object=F.Object;I.Range=3.2;
  I.Label=[this,Guard,Id]{return Guard.IsValid()?Label(Id):FString();};
  I.Use=[this,Guard,Id]{if(Guard.IsValid())Use(Id);};
  I.Examine=I.Label;
  M->Inter->Add(I);M->Web->MarkUsable(F.Object.I,Id);
 }
}

void FS47RoomDecor::Show(bool bShow)
{
 for(const FFrame& F:Frames)for(const auto& C:{F.Border,F.Picture})if(C.IsValid())
 {
  C->SetVisibility(bShow);C->SetHiddenInGame(!bShow);
  if(C==F.Border)C->SetCollisionEnabled(bShow?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 }
}

void FS47RoomDecor::Begin(const S47J::FObj& Saved)
{
 Stop();if(!M||!IsValid(M->Web))return;
 if(Scene.Get()!=M->Web){bBuilt=false;Frames.Reset();}
 Scene=M->Web;Life=MakeShared<uint8>(0);bActive=true;
 Pending=Saved.IsValid()?S47J::Strs(Saved,TEXT("frames")):TArray<FString>();
 if(!Build())return;
 Bind();Show(true);
 for(FFrame& F:Frames)F.PhotoId.Empty();
 bApply=true;
}

void FS47RoomDecor::Stop()
{
 bActive=false;bApply=false;Life.Reset();Show(false);
}

void FS47RoomDecor::Step()
{
 // The album is restored after the session begins, so the prints go up on the first tick that can see it.
 if(!bActive||!bApply||!M||!M->Night||!M->Night->Album)return;
 bApply=false;
 for(int32 I=0;I<Frames.Num();++I){Frames[I].PhotoId=Pending.IsValidIndex(I)?Pending[I]:FString();Apply(Frames[I]);}
 Pending.Reset();
}

S47J::FObj FS47RoomDecor::Save() const
{
 const auto O=S47J::New();TArray<FString> Ids;
 if(bApply)Ids=Pending;else for(const FFrame& F:Frames)Ids.Add(F.PhotoId);
 S47J::Set(O,TEXT("frames"),Ids);
 return O;
}

const FS47AlbumShot* FS47RoomDecor::Shot(const FString& PhotoId) const
{
 if(PhotoId.IsEmpty()||!M||!M->Night||!M->Night->Album)return nullptr;
 return M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& S){return S.Id==PhotoId;});
}

void FS47RoomDecor::Apply(FFrame& Frame)
{
 const FS47AlbumShot* S=Shot(Frame.PhotoId);
 UTexture2D* Texture=S&&S->IsDeveloped()&&!S->bUnavailable?S47Image::FromDataUrl(S->Url):nullptr;
 if(!Texture)Frame.PhotoId.Empty(); // A print that is gone leaves an empty frame, never a stale picture.
 if(auto* Mat=Frame.Material.Get())
 {
  Mat->SetVectorParameterValue(TEXT("Color"),Texture?FLinearColor::White:FLinearColor(.72,.68,.58));
  Mat->SetScalarParameterValue(TEXT("UseMap"),Texture?1.f:0.f);Mat->SetScalarParameterValue(TEXT("MapIsLinear"),0.f);
  if(Texture)Mat->SetTextureParameterValue(TEXT("Map"),Texture);
  Mat->SetVectorParameterValue(TEXT("MapU"),FLinearColor(1,0,0,0));Mat->SetVectorParameterValue(TEXT("MapV"),FLinearColor(0,1,0,0));
  // Night prints would go black on a dim wall; a little of their own light keeps them readable.
  Mat->SetScalarParameterValue(TEXT("UseEmissiveMap"),Texture?1.f:0.f);
  if(Texture)
  {
   Mat->SetTextureParameterValue(TEXT("EmissiveMap"),Texture);Mat->SetVectorParameterValue(TEXT("Emissive"),FLinearColor(.45,.45,.45));
   Mat->SetVectorParameterValue(TEXT("EmU"),FLinearColor(1,0,0,0));Mat->SetVectorParameterValue(TEXT("EmV"),FLinearColor(0,1,0,0));
  }
  else Mat->SetVectorParameterValue(TEXT("Emissive"),FLinearColor::Black);
 }
}

bool FS47RoomDecor::Hang(int32 Slot,const FString& PhotoId)
{
 if(!Frames.IsValidIndex(Slot))return false;
 const FS47AlbumShot* S=Shot(PhotoId);if(!S||!S->IsDeveloped()||S->bUnavailable)return false;
 for(FFrame& F:Frames)if(F.PhotoId==PhotoId){F.PhotoId.Empty();Apply(F);} // One print, one frame.
 Frames[Slot].PhotoId=PhotoId;Apply(Frames[Slot]);
 M->RequestAutosave();
 return true;
}

void FS47RoomDecor::TakeDown(int32 Slot)
{
 if(!Frames.IsValidIndex(Slot))return;
 Frames[Slot].PhotoId.Empty();Apply(Frames[Slot]);
 M->RequestAutosave();
}

bool FS47RoomDecor::HoldsPhoto(const FString& PhotoId) const
{
 return !PhotoId.IsEmpty()&&(Pending.Contains(PhotoId)||Frames.ContainsByPredicate([&](const FFrame& F){return F.PhotoId==PhotoId;}));
}

FVector FS47RoomDecor::SlotPosition(int32 Slot) const
{
 return Frames.IsValidIndex(Slot)?Frames[Slot].Object.WorldPosition():FVector::ZeroVector;
}

FString FS47RoomDecor::Label(const FString& Id) const
{
 const FFrame* F=Frames.FindByPredicate([&](const FFrame& X){return X.Id==Id;});
 if(!F||!bActive)return FString();
 return F->PhotoId.IsEmpty()?TEXT("Empty frame / hang a photograph"):TEXT("Your photograph / change it");
}

bool FS47RoomDecor::Use(const FString& Id)
{
 const int32 Slot=Frames.IndexOfByPredicate([&](const FFrame& X){return X.Id==Id;});
 if(Slot==INDEX_NONE||!bActive)return false;
 OpenPanel(Slot);return true;
}

void FS47RoomDecor::OpenPanel(int32 Slot)
{
 if(!M||!M->UI||!M->Night||!M->Night->Album)return;
 TArray<S47Kit::FW> Rows;
 const auto Close=[this]{if(M&&M->UI)M->UI->Close();};
 if(!Frames[Slot].PhotoId.IsEmpty())Rows.Add(S47Kit::Button(TEXT("Take this photograph down"),[this,Slot,Close]{TakeDown(Slot);Close();}));
 const TSharedRef<SScrollBox> List=SNew(SScrollBox);int32 Count=0;
 const auto& Shots=M->Night->Album->Shots();
 for(int32 I=Shots.Num()-1;I>=0;--I)
 {
  const FS47AlbumShot& S=Shots[I];
  if(!S.IsDeveloped()||S.bUnavailable||S.Id==Frames[Slot].PhotoId)continue;
  UTexture2D* Texture=S47Image::FromDataUrl(S.Url);if(!Texture)continue;
  const FString PhotoId=S.Id;++Count;
  List->AddSlot().Padding(0,0,0,8)
  [
   S47Kit::Two(S47Kit::Image(Texture,200.f),S47Kit::Col({
    S47Kit::Small(S.Place.IsEmpty()?S.Text:S.Place+TEXT(" / ")+S.Text),
    S47Kit::Button(HoldsPhoto(PhotoId)?TEXT("Move here"):TEXT("Hang here"),[this,Slot,PhotoId,Close]{Hang(Slot,PhotoId);Close();})}),.32f,.68f)
  ];
 }
 if(Count)Rows.Add(SNew(SBox).MaxDesiredHeight(520)[List]);
 else Rows.Add(S47Kit::Text(TEXT("No developed photographs to hang. Expose film with the field camera and develop it at the wet bench.")));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO / WALL"),S47Kit::Col(Rows),860));
}

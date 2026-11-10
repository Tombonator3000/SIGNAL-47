#include "S47Darkroom.h"
#include "S47Album.h"
#include "S47Audio.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"

// D45-20261110 (D45-17): the switch, the rule and the fogged print. See S47Darkroom.h.
namespace S47DarkroomLocal
{
 // The lab's west wall, inside (its face is x = 12.8, ServiceYard.ts), just north of the door frame: clear of the door's sweep
 // (0.93 m round the hinge at z = -0.73), the film shelves and the utility fittings (z -3.05 and beyond). Metres, gameplay coordinates.
 const FVector DkSwitchAt(12.80, 1.35, -2.05);
 const FVector DkFacing(1, 0, 0);
 const TCHAR* const DkParent = TEXT("yard.occluders[2]"); // the lab group, as S47PhotoLabDetail uses
 const TCHAR* const DkDoor = TEXT("labDoor");
}
using namespace S47DarkroomLocal;
const TCHAR* const FS47Darkroom::SwitchId = TEXT("darkroom:safelight");

FS47Darkroom::~FS47Darkroom(){Stop();Life.Reset();}

// ---------- the pure rules ----------

FString FS47Darkroom::BlockFor(bool bPower, bool bDoorOpen, bool bSafeOn)
{
 if(!bPower)return TEXT("PHOTO LAB: No power. Restore it before you develop. Your film is safe.");
 if(bDoorOpen&&!bSafeOn)return TEXT("PHOTO LAB: Close the door and switch on the safelight first. The switch is by the door.");
 if(bDoorOpen)return TEXT("PHOTO LAB: Close the door first. Light from outside ruins the print.");
 if(!bSafeOn)return TEXT("PHOTO LAB: Switch on the safelight first. The switch is by the door. White light ruins the print.");
 return FString();
}
// Only a change seen live ruins a print: the door opening on the red room, or the switch going back to white. Power is not an
// input here: a cut only darkens the room. (Under white light already, as an old save's print in the tank, the door adds nothing.)
bool FS47Darkroom::FogsOn(bool bWasDoor, bool bDoorNow, bool bWasSafe, bool bSafeNow, bool bAtRisk)
{
 return bAtRisk&&((bDoorNow&&!bWasDoor&&bSafeNow)||(bWasSafe&&!bSafeNow));
}

// ---------- the session ----------

void FS47Darkroom::Begin(const S47J::FObj& Saved)
{
 bActive=true;bSafe=S47J::Bool(Saved,TEXT("safelight"),false);
 // The lights follow the saved switch once the world is in; white is the lab's own default, so only a change is written.
 bPendingApply=bSafe||bApplied;
 bPrimed=false;PrimeWait=0;Retry=0;Fogged.Reset();Shown=-1;
}
S47J::FObj FS47Darkroom::Save() const
{
 auto O=S47J::New();
 if(bSafe){S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("safelight"),true);} // white (the default) writes nothing: old saves' shape
 return O;
}
FString FS47Darkroom::Block() const
{
 if(!M||!M->Freeform||!M->Web)return TEXT("PHOTO LAB: Not available.");
 return BlockFor(M->Freeform->HasPowerForReactiveLight(),M->Web->IsDoorOpen(DkDoor),bSafe);
}
bool FS47Darkroom::PrintAtRisk() const
{
 if(!M||!M->Night||!M->Night->Album)return false;
 return M->Night->Album->Shots().ContainsByPredicate([](const FS47AlbumShot& Sh){return Sh.LabStage==1&&!Sh.bUnavailable&&!Sh.Url.IsEmpty();});
}
void FS47Darkroom::SetSafelight(bool bOn, bool bSilent)
{
 if(bOn==bSafe)return;
 const bool bWasSafe=bSafe;bSafe=bOn;Apply();
 // White light only ruins the print when the tubes can actually light.
 const bool bPowered=M&&M->Freeform&&M->Freeform->HasPowerForReactiveLight();
 if(bPowered&&FogsOn(false,false,bWasSafe,bSafe,PrintAtRisk()))Fog();
 else if(!bSilent&&M&&M->UI)
  M->UI->Toast(FString(bOn?TEXT("Safelight on. The white light is off, and the lamp outside the door says DARKROOM IN USE."):TEXT("White light on. The safelight is off."))
   +(bPowered?TEXT(""):TEXT(" No power right now. The switch stays where you left it.")),3.5f);
 if(!bSilent&&M&&M->Audio){FS47Sound S;S.At=InteractionPoint();S.Gain=.45f;M->Audio->Sfx(M->Audio->Or(TEXT("small_switch"),TEXT("switch")),S);}
 if(M)M->RequestAutosave();
}
void FS47Darkroom::Apply()
{
 bPendingApply=false;
 // The named controls exist once the lab's lighting is built (S47LabLighting); until then, try again next frame.
 const FS47Light Normal=M&&M->Web&&M->Web->IsLoaded()?M->Web->Light(TEXT("lab.normal")):FS47Light(),Safe=M&&M->Web&&M->Web->IsLoaded()?M->Web->Light(TEXT("lab.safelight")):FS47Light();
 if(!Normal||!Safe){bPendingApply=M!=nullptr;return;}
 // Edge only, never every frame: the review views and Story write the same named controls.
 Normal.SetIntensity(bSafe?0.f:1.f);Safe.SetIntensity(bSafe?1.f:0.f);
 bApplied=bSafe;
 Refresh(true);
}
void FS47Darkroom::Fog()
{
 if(!M||!M->Night||!M->Night->Album)return;
 TArray<FString> Ids;
 for(const FS47AlbumShot& Sh:M->Night->Album->Shots())if(Sh.LabStage==1)Ids.Add(Sh.Id);
 bool bAny=false;
 for(const FString& Id:Ids)if(M->Night->Album->FogLabPhoto(Id)){Fogged.Add(Id);bAny=true;}
 if(!bAny)return;
 if(M->Audio)M->Audio->Beep(330.f,.4f,.04f,.2f,TEXT("sine")); // the cue Story's fogged print uses
 if(M->UI)M->UI->Toast(TEXT("Light got into the darkroom and ruined the print in the developer. The negative is fine. Make a new print at the bench; it costs nothing."),5.f);
 M->RequestAutosave();
}
void FS47Darkroom::Step(double Dt)
{
 if(!bActive||!M||!M->Web||!M->Web->IsLoaded())return;
 if(!bBuilt){Retry-=FMath::Max(0.,Dt);if(Retry<=0){Build();Retry=1.;}}
 if(bPendingApply)Apply();
 Refresh(false);
 const bool bDoor=M->Web->IsDoorOpen(DkDoor);
 // After Begin or Continue the doors are put back a few frames late: watch only after a quiet second, so a loaded state never fogs.
 if(M->Flow&&(M->Flow->bStarting||M->Flow->bRestoringNow)){bPrimed=false;PrimeWait=0;}
 if(!bPrimed){PrimeWait+=FMath::Max(0.,Dt);bLastDoor=bDoor;bPrimed=PrimeWait>=1.;return;}
 if(FogsOn(bLastDoor,bDoor,bSafe,bSafe,PrintAtRisk()))Fog();
 bLastDoor=bDoor;
}
void FS47Darkroom::Stop()
{
 bActive=false;Fogged.Reset();
 if(IsEngineExitRequested()||IsGarbageCollecting()||!IsValid(M)||M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)||!IsValid(M->Web.Get()))return;
 // Story and a new case start with the lab's own white light.
 if(bApplied&&M->Web->IsLoaded()){M->Web->Light(TEXT("lab.normal")).SetIntensity(1.f);M->Web->Light(TEXT("lab.safelight")).SetIntensity(0.f);}
 bApplied=false;
 Refresh(true);
}

// ---------- the switch ----------

FVector FS47Darkroom::InteractionPoint() const {return DkSwitchAt+DkFacing*.028;}
bool FS47Darkroom::Available() const
{
 return bActive&&bBuilt&&M&&M->IsFreeform()&&M->Mode==US47Main::EMode::Play&&M->Web&&M->World&&M->World->Area==TEXT("saro")&&!M->World->bDriving&&M->Web->Obj(DkParent).Shown();
}
FString FS47Darkroom::Label() const
{
 if(!Available())return FString();
 return FString(bSafe?TEXT("Safelight switch / turn OFF, white light on"):TEXT("Safelight switch / turn ON, white light off"))+(bSafe&&PrintAtRisk()?TEXT(" / a print is in the developer"):TEXT(""));
}
bool FS47Darkroom::Use()
{
 if(!Available()||!M->Player||!M->Inter)return false;
 if(M->bPausedByMenu||M->ModalOpen()||(M->Story&&M->Story->IsCinematic()))return false;
 const FVector Eye=S47Three::ToThree(M->Player->EyeUE());
 // Distance plus the actual eye ray, as the room switches: no remote use, none through the wall from the yard.
 if(FVector::DotProduct(Eye-DkSwitchAt,DkFacing)<=0||FVector::Dist(Eye,InteractionPoint())>2.2||M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE())!=SwitchId)return false;
 SetSafelight(!bSafe,false);return true;
}
bool FS47Darkroom::Build()
{
 if(bBuilt)return true;
 if(!M||!M->Web||!M->Web->IsLoaded()||!M->Inter)return false;
 const FS47Obj Parent=M->Web->Obj(DkParent);
 if(!Parent||!M->Web->Node(Parent.I))return false;
 auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 if(!Mesh||!Base)return false;
 // The plate and lever as the room switches' (S47RoomLights.cpp), on the lab's side wall.
 UMaterialInstanceDynamic* Made=nullptr;
 auto Box=[&](const TCHAR* Name,const FVector& At,const FVector& Size,uint32 Color)
 {
  auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web);Made=Mat;Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Color));Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);Mat->SetScalarParameterValue(TEXT("Roughness"),.7f);
  auto* C=NewObject<UStaticMeshComponent>(M->Web,MakeUniqueObjectName(M->Web,UStaticMeshComponent::StaticClass(),FName(Name)));C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->Node(Parent.I)->Comp.Get());C->SetStaticMesh(Mesh);C->SetMaterial(0,Mat);
  C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldScale3D(S47Three::DirToUE(Size));C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);C->RegisterComponent();
  FS47Obj O=M->Web->ObjAt(M->Web->AdoptNode(C,Parent.I,Name));Parts.Add(O);M->Web->MarkUsable(O.I,SwitchId);return O;
 };
 Plate=Box(TEXT("DarkroomSwitchPlate"),DkSwitchAt,FVector(.022,.15,.09),0xc3baa5);
 Lever=Box(TEXT("DarkroomSwitchLever"),InteractionPoint(),FVector(.026,.055,.025),0xe0d8c5);
 Pilot=Made;if(Made){Made->SetVectorParameterValue(TEXT("Emissive"),S47Three::Hex(0xff2616));Made->SetScalarParameterValue(TEXT("EmissivePower"),0);} // the red pilot in the lever
 auto* Text=NewObject<UTextRenderComponent>(M->Web,MakeUniqueObjectName(M->Web,UTextRenderComponent::StaticClass(),FName(TEXT("DarkroomSwitchText"))));
 Text->SetupAttachment(M->Web->Node(Parent.I)->Comp.Get());Text->SetMobility(EComponentMobility::Movable);Text->SetText(FText::FromString(TEXT("DARKROOM\nSAFELIGHT")));Text->SetTextRenderColor(FColor(212,203,179));Text->SetWorldSize(1.6f);Text->SetHorizontalAlignment(EHTA_Center);Text->SetVerticalAlignment(EVRTA_TextCenter);Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);Text->RegisterComponent();
 Text->SetWorldLocation(S47Three::ToUE(DkSwitchAt+FVector(0,.14,0)+DkFacing*.016));Text->SetWorldRotation(FRotator(0,0,0)); // faces +x, into the lab
 Parts.Add(M->Web->ObjAt(M->Web->AdoptNode(Text,Parent.I,TEXT("DarkroomSwitchLabel"))));
 const TWeakPtr<uint8> Guard=Life;
 FS47Interactable Item;Item.Id=SwitchId;Item.Object=Plate;Item.Range=2.2;
 Item.Label=[this,Guard]{return Guard.IsValid()?Label():FString();};
 Item.Use=[this,Guard]{if(Guard.IsValid())Use();};
 Item.Examine=[this,Guard]
 {
  if(!Guard.IsValid()||!Available())return FString();
  return FString::Printf(TEXT("The darkroom switch, %s. Down: the white room light. Up: the red safelights, and the DARKROOM IN USE lamp outside the door. Photo paper is only safe under the red light."),bSafe?TEXT("up"):TEXT("down"));
 };
 M->Inter->Add(Item);
 bBuilt=true;Shown=-1;Refresh(true);return true;
}
// Visibility, the lever's throw and its pilot: only when something changed.
void FS47Darkroom::Refresh(bool bForce)
{
 if(!bBuilt||!M||!M->Web)return;
 const bool bShow=Available();
 const int32 Now=(bShow?1:0)|(bSafe?2:0);
 if(!bForce&&Now==Shown)return;
 Shown=Now;
 for(FS47Obj& P:Parts)if(P)
 {
  P.SetVisible(bShow);
  if(const auto* N=M->Web->Node(P.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))C->SetCollisionEnabled(bShow&&C->IsA<UStaticMeshComponent>()?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 }
 if(auto* Glow=Pilot.Get())Glow->SetScalarParameterValue(TEXT("EmissivePower"),bShow&&bSafe?4.f:0.f);
 // Adopted Web nodes do not keep the native mesh scale; rotate the component, as the room switches do.
 if(const auto* N=M->Web->Node(Lever.I))if(auto* C=N->Comp.Get())C->SetWorldRotation(FRotator(bSafe?-22.:22.,0,0));
}

// ---------- tests ----------

bool FS47Darkroom::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto Check=[&](bool Good,const TCHAR* Message){OK&=Good;Lines.Add(FString(Good?TEXT("PASS "):TEXT("FAIL "))+TEXT("darkroom: ")+Message);};
 bool OnlyReady=true;
 for(int32 Mask=0;Mask<8;++Mask)
 {
  const bool bP=(Mask&1)!=0,bD=(Mask&2)!=0,bS=(Mask&4)!=0;
  OnlyReady&=BlockFor(bP,bD,bS).IsEmpty()==(bP&&!bD&&bS);
 }
 Check(OnlyReady,TEXT("of the 8 power/door/safelight states only power, door shut and safelight on lets the print be made"));
 Check(BlockFor(true,true,false)==TEXT("PHOTO LAB: Close the door and switch on the safelight first. The switch is by the door.")
  &&BlockFor(false,true,false).Contains(TEXT("No power"))&&BlockFor(true,true,true).Contains(TEXT("Close the door first"))&&BlockFor(true,false,false).Contains(TEXT("Switch on the safelight first")),
  TEXT("each blocked state says the one thing to do"));
 Check(FogsOn(false,true,true,true,true)&&!FogsOn(true,false,true,true,true)&&!FogsOn(true,true,true,true,true),TEXT("the door opening ruins a print in the developer; closing it or keeping it open does not"));
 Check(!FogsOn(false,true,false,false,true),TEXT("an old save's print under white light is not ruined again by the door"));
 Check(FogsOn(false,false,true,false,true)&&!FogsOn(false,false,false,true,true),TEXT("the switch back to white ruins a print in the developer; on to red does not"));
 bool NoRisk=true;for(int32 Mask=0;Mask<16;++Mask)NoRisk&=!FogsOn((Mask&1)!=0,(Mask&2)!=0,(Mask&4)!=0,(Mask&8)!=0,false);
 Check(NoRisk,TEXT("with no print in the developer nothing is ever ruined"));
 FS47Darkroom D(nullptr);D.Begin();
 Check(!D.SafelightOn()&&D.Save()->Values.Num()==0,TEXT("an old save has white light and the white switch writes nothing"));
 D.SetSafelight(true,true);
 FS47Darkroom R(nullptr);R.Begin(S47J::Read(S47J::Write(D.Save())));
 Check(D.SafelightOn()&&R.SafelightOn()&&S47J::Bool(D.Save(),TEXT("safelight")),TEXT("the safelight switch survives JSON Continue"));
 R.SetSafelight(false,true);FS47Darkroom W(nullptr);W.Begin(S47J::Read(S47J::Write(R.Save())));
 Check(!R.SafelightOn()&&!W.SafelightOn(),TEXT("switched back to white, it stays white after Continue"));
 Check(!D.Use()&&D.Label().IsEmpty()&&!D.PrintAtRisk()&&!D.Block().IsEmpty(),TEXT("without a world the switch cannot be used and the bench stays blocked"));
 D.Stop();D.Begin();Check(!D.SafelightOn(),TEXT("a new case starts with white light"));
 return OK;
}

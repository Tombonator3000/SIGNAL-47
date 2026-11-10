#include "S47Balloon.h"
#include "S47Audio.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47Happenings.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Legs.h"
#include "S47Mail.h"
#include "S47Main.h"
#include "S47Places.h"
#include "S47Script.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "PhysicsEngine/BodySetup.h"

namespace
{
 // The four pieces, as Tools/balloon/build_balloon.py lays them out and import_assets.py names them. Offsets from the place in metres
 // on its own ground (x east, y up, z south: Blender's north is south here, so its y comes in with the sign turned), each piece's pivot
 // on the ground under its own middle. The cord runs from the envelope's neck toward the parachute's ring.
 struct FLay{const TCHAR* Name;FVector At;float Yaw;};
 const FLay Layout[]={{TEXT("envelope"),{0,0,0},0},{TEXT("parachute"),{2.9,0,1.9},0},{TEXT("cord"),{-1.75,0,1.0},8.5f},{TEXT("radiosonde"),{1.9,0,1.45},0}};
 // Where the beacon's lamp sits on the radiosonde, from the manifest (balloon.json beacon_m, Blender x, y, z up: here x, up, -y).
 const FVector BeaconOnBox(.087,.107,.0011);
 // The box as a basic shape where the model is not: 20 x 10 x 15 cm, pale, a little turned, and its lamp on the top corner.
 const FVector PlainBoxSize(.2,.1,.15),PlainLampAt(.07,.115,-.045);
 const TCHAR* const Cube=TEXT("/Engine/BasicShapes/Cube.Cube");
 const TCHAR* const Ball=TEXT("/Engine/BasicShapes/Sphere.Sphere");
 const TCHAR* const Lit=TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque");
 const TCHAR* StateName(FS47Balloon::EBox B)
 {
  switch(B){case FS47Balloon::EBox::Carried:return TEXT("carried");case FS47Balloon::EBox::Mailed:return TEXT("mailed");case FS47Balloon::EBox::Promised:return TEXT("promised");case FS47Balloon::EBox::Gone:return TEXT("gone");default:return TEXT("lying");}
 }
 FS47Balloon::EBox StateOf(const FString& S)
 {
  for(const auto B:{FS47Balloon::EBox::Carried,FS47Balloon::EBox::Mailed,FS47Balloon::EBox::Promised,FS47Balloon::EBox::Gone})if(S==StateName(B))return B;
  return FS47Balloon::EBox::Lying;
 }
}

bool FS47Balloon::HaveModel()
{
 for(const FLay& L:Layout)if(!FPackageName::DoesPackageExist(FString(Folder)+L.Name))return false;
 return true;
}
UStaticMesh* FS47Balloon::Piece(const TCHAR* Name) const
{
 // As FS47FieldLife's TripModel: only with every material bound and collision to answer the use trace; otherwise nothing.
 const FString Package=FString(Folder)+Name;
 if(!FPackageName::DoesPackageExist(Package))return nullptr;
 UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*(Package+TEXT(".")+Name));
 if(!Mesh||Mesh->GetStaticMaterials().IsEmpty())return nullptr;
 for(const FStaticMaterial& Slot:Mesh->GetStaticMaterials())if(!Slot.MaterialInterface){UE_LOG(LogTemp,Warning,TEXT("S47: balloon piece %s has an unbound material slot; the basic shape stands for it"),Name);return nullptr;}
 const UBodySetup* Body=Mesh->GetBodySetup();
 if(!Body||(Body->AggGeom.GetElementCount()==0&&Body->GetCollisionTraceFlag()!=CTF_UseComplexAsSimple)){UE_LOG(LogTemp,Warning,TEXT("S47: balloon piece %s has no collision (import it again); the basic shape stands for it"),Name);return nullptr;}
 return Mesh;
}
void FS47Balloon::Begin(const S47J::FObj& Saved)
{
 Stop();Life=MakeShared<uint8>(0);
 Box=EBox::Lying;HandledDay=0;SchoolClock=-1;bNoted=false;T=0;SeatIn=0;bSeated=false;
 if(Saved.IsValid())
 {
  Box=StateOf(S47J::Str(Saved,TEXT("box")));
  HandledDay=FMath::Clamp(S47J::Int(Saved,TEXT("day")),0,36500);
  const double Due=S47J::Num(Saved,TEXT("school"),-1);
  SchoolClock=FMath::IsFinite(Due)&&Due>=0?Due:-1;
  if(Box==EBox::Promised&&SchoolClock<0)Box=EBox::Carried; // (nonsense: no hour to come by: still in hand)
  bNoted=S47J::Bool(Saved,TEXT("noted"));
 }
 if(!M||!IsValid(M->Web.Get()))return;
 if(Scene.Get()!=M->Web.Get()){bBuilt=false;Parts.Reset();Lamp.Reset();Glow.Reset();}
 Scene=M->Web;Build();Show();
}
void FS47Balloon::Stop(){Life.Reset();}
S47J::FObj FS47Balloon::Save() const
{
 auto O=S47J::New();
 if(Box!=EBox::Lying)S47J::Set(O,TEXT("box"),StateName(Box));
 if(HandledDay>0)S47J::Set(O,TEXT("day"),HandledDay);
 if(SchoolClock>=0)S47J::Set(O,TEXT("school"),SchoolClock);
 if(bNoted)S47J::Set(O,TEXT("noted"),true);
 return O;
}
UStaticMeshComponent* FS47Balloon::Part(UStaticMesh* Mesh,const FString& Name,const FVector& At,const FVector& Scale,float Yaw,UMaterialInterface* Mat,bool bBox)
{
 auto* C=NewObject<UStaticMeshComponent>(M->Web.Get(),*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(Mesh);
 C->SetForceDisableNanite(true);C->SetCastShadow(true); // (the pieces come in without Nanite: their glTF materials are not marked for it, D44)
 if(Mat)C->SetMaterial(0,Mat);
 C->SetWorldScale3D(Scale);C->SetWorldRotation(FRotator(0,Yaw,0));
 C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
 C->RegisterComponent();
 FPart P;P.C=C;P.At=At;P.bBox=bBox;Parts.Add(P);
 return C;
}
bool FS47Balloon::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web.Get())||!IsValid(M->Inter)||!M->Freeform||!M->Freeform->Places)return false;
 const int32 RoadRoot=M->Web->Handle(TEXT("world.legs.road.area.group"));if(RoadRoot<0)return false;
 auto* Base=LoadObject<UMaterialInterface>(nullptr,Lit);auto* CubeMesh=LoadObject<UStaticMesh>(nullptr,Cube);auto* BallMesh=LoadObject<UStaticMesh>(nullptr,Ball);
 if(!Base||!CubeMesh||!BallMesh)return false;
 const TWeakPtr<uint8> Guard=Life;
 bModel=HaveModel();
 FS47Obj Look;
 if(bModel)
 {
  // The model: the three pieces that stay and the box. Any piece that will not load (no collision, a slot unbound) is left out: the
  // find is then thinner, never wrong.
  for(const FLay& L:Layout)
  {
   UStaticMesh* Mesh=Piece(L.Name);if(!Mesh)continue;
   const bool bBox=FCString::Strcmp(L.Name,TEXT("radiosonde"))==0;
   auto* C=Part(Mesh,FString::Printf(TEXT("S47Balloon_%s"),L.Name),L.At,FVector(1,1,1),L.Yaw,nullptr,bBox);
   const FString Use=bBox?FString(TEXT("ff:balloon:box")):FS47Places::UseId(PlaceId);
   const FS47Obj O=M->Web->ObjAt(M->Web->AdoptNode(C,RoadRoot,bBox?Use:FString::Printf(TEXT("ff:balloon:%s"),L.Name)));
   if(O)M->Web->MarkUsable(O.I,Use);
   if(bBox){LampAt=L.At+BeaconOnBox;BoxObj=O;}
   else if(!Look&&FCString::Strcmp(L.Name,TEXT("envelope"))==0)Look=O;
  }
  if(Look)
  {
   // "look closer" on the rubber, as the basic shapes gave it (FS47Places::Build gives none when the model draws the place)
   FS47Interactable I;I.Id=FS47Places::UseId(PlaceId);I.Object=Look;I.Range=6;
   I.Label=[Guard]{return Guard.IsValid()&&FS47Places::Def(PlaceId)?FString::Printf(TEXT("%s / look closer"),FS47Places::Def(PlaceId)->Title):FString();};
   I.Use=[this,Guard]{if(Guard.IsValid()&&M&&M->Freeform&&M->Freeform->Places)M->Freeform->Places->Use(PlaceId);};
   I.Examine=I.Label;M->Inter->Add(I);
  }
  UE_LOG(LogTemp,Display,TEXT("S47: balloon drawn from %s (%d pieces)"),Folder,Parts.Num());
 }
 if(!Parts.ContainsByPredicate([](const FPart& P){return P.bBox;}))
 {
  // The box as a pale basic shape (the place's rubber, chute and cord are FS47Places' own basic shapes then).
  auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web.Get());Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(0xd8d4c6));Mat->SetScalarParameterValue(TEXT("Roughness"),.92f);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  const FVector At(1.9,PlainBoxSize.Y/2,1.45);
  auto* C=Part(CubeMesh,TEXT("S47Balloon_box"),At,FVector(PlainBoxSize.X,PlainBoxSize.Z,PlainBoxSize.Y),15,Mat,true);
  const FS47Obj O=M->Web->ObjAt(M->Web->AdoptNode(C,RoadRoot,TEXT("ff:balloon:box")));if(O)M->Web->MarkUsable(O.I,TEXT("ff:balloon:box"));
  LampAt=FVector(1.9,0,1.45)+PlainLampAt;BoxObj=O;
 }
 // The beacon: a small red lamp (its emissive pulses) and a low point light with no shadows at it.
 {
  auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web.Get());Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.5f,.05f,.03f));Mat->SetScalarParameterValue(TEXT("Roughness"),.25f);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  Mat->SetVectorParameterValue(TEXT("Emissive"),FLinearColor(1.f,.1f,.05f));Mat->SetScalarParameterValue(TEXT("EmissivePower"),0.f);Glow=Mat;
  auto* C=Part(BallMesh,TEXT("S47Balloon_lamp"),LampAt,FVector(.026,.026,.02),0,Mat,true);
  Parts.Last().bLamp=true;C->SetCastShadow(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  auto* L=NewObject<UPointLightComponent>(M->Web.Get(),TEXT("S47Balloon_beacon"));L->SetupAttachment(C);L->SetMobility(EComponentMobility::Movable);
  L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(0);L->SetLightColor(FLinearColor(1.f,.18f,.08f));L->SetAttenuationRadius(900);L->SetCastShadows(false);
  L->RegisterComponent();Lamp=L;
 }
 // E on the box: into the hand.
 {
  FS47Interactable I;I.Id=TEXT("ff:balloon:box");I.Object=BoxObj;I.Range=3;
  I.Label=[this,Guard]{return Guard.IsValid()&&Box==EBox::Lying?FString(TEXT("Instrument box / take it")):FString();};
  I.Use=[this,Guard]{if(Guard.IsValid())Take();};
  I.Examine=[this,Guard]{return Guard.IsValid()&&Box==EBox::Lying?FString(TEXT("A cardboard and foam box the size of a shoebox, a wire antenna, a battery pack on a lead. A little red lamp on the top corner blinks. The label asks the finder to mail it in, postage paid.")):FString();};
  M->Inter->Add(I);
 }
 bBuilt=true;Seat(true);
 return true;
}
void FS47Balloon::Seat(bool bForce)
{
 if(!M||!M->Freeform||!M->Freeform->Places)return;
 FVector World;bool bOnGround=false;
 if(!M->Freeform->Places->Stand(PlaceId,World,bOnGround))return;
 if(!bForce&&bSeated&&World.Equals(Stand,.005))return;
 Stand=World;bSeated=bOnGround;
 for(const FPart& P:Parts)if(auto* C=P.C.Get())C->SetWorldLocation(S47Three::ToUE(World+P.At));
}
void FS47Balloon::Show()
{
 const bool bThere=Box==EBox::Lying;
 for(const FPart& P:Parts)if(P.bBox)if(auto* C=P.C.Get()){C->SetVisibility(bThere);C->SetHiddenInGame(!bThere);C->SetCollisionEnabled(bThere&&!P.bLamp?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}
 if(!bThere){if(auto* L=Lamp.Get())L->SetIntensity(0);if(auto* G=Glow.Get())G->SetScalarParameterValue(TEXT("EmissivePower"),0.f);bLit=false;}
}
double FS47Balloon::Blink(double Time,bool bNight)
{
 if(!FMath::IsFinite(Time))return 0;
 const double Phase=FMath::Fmod(FMath::Fmod(Time,BlinkPeriod)+BlinkPeriod,BlinkPeriod);
 const double On=Phase<BlinkOn?1.:0.;
 return bNight?On:On*DayGlow;
}
bool FS47Balloon::SchoolOpen(double Clock)
{
 if(!FMath::IsFinite(Clock))return false;
 const double H=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;
 return H>=8&&H<16;
}
double FS47Balloon::SchoolDue(double Clock)
{
 const double Day0=FMath::FloorToDouble(FMath::Max(0.,Clock)/86400.)*86400.,Hour=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;
 return Day0+(Hour<8?0:86400.)+15*3600.;
}
void FS47Balloon::Step(double Dt)
{
 if(!M||!M->Freeform||!M->Web)return;
 if(!bBuilt&&!Build())return;
 auto& F=*M->Freeform;
 SeatIn-=Dt;if(SeatIn<=0){SeatIn=.5;Seat(false);}
 // The school's pickup: they come by SARO the afternoon after the call, whether the player is there or not, and leave the money.
 if(Box==EBox::Promised&&SchoolClock>=0&&F.Clock()>=SchoolClock)
 {
  Box=EBox::Gone;HandledDay=F.Day();
  F.Earn(SchoolReward);
  F.RecordEvidence(TEXT("place_balloon_school"),FString::Printf(TEXT("The high school's pickup came by SARO for the balloon's instrument box, as the science teacher said on the phone. Ten dollars in an envelope on the OPS desk, from the science club's jar, and a note: they get one a year if they are lucky.")));
  if(M->UI)M->UI->Toast(FString::Printf(TEXT("The high school came by for the balloon box while you were at the dishes. $%d in an envelope on the OPS desk."),SchoolReward),7);
  UE_LOG(LogTemp,Display,TEXT("S47: balloon: the school's pickup took the box, $%d"),SchoolReward);
  M->RequestAutosave();
 }
 if(Box!=EBox::Lying)return;
 // The beacon, about every 1.8 s, bright at night and faint by day; nothing is spent on it while the player is far off the land.
 T+=Dt;
 const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.;
 const bool bNight=Hour<6.5||Hour>=19;
 const double K=Blink(T,bNight);const bool bOn=K>0;
 if(bOn==bLit)return;
 bLit=bOn;
 if(auto* G=Glow.Get())G->SetScalarParameterValue(TEXT("EmissivePower"),float(K*60.));
 if(auto* L=Lamp.Get())L->SetIntensity(float(K*35.));
}
bool FS47Balloon::Take()
{
 if(Box!=EBox::Lying||!M||!M->Freeform)return false;
 auto& F=*M->Freeform;
 Box=EBox::Carried;HandledDay=F.Day();Show();
 if(M->Audio)M->Audio->Sfx(TEXT("paper1"),FS47Sound{.Gain=.45f});
 F.RecordEvidence(TEXT("place_balloon_box"),BoxRecord());
 Noted();
 if(M->UI)M->UI->Toast(TEXT("You take the instrument box; the little lamp goes dark in your hand. It rides with your kit. The label asks for it by mail, postage paid; the Ledger's ad says the high school will come for it."),8);
 UE_LOG(LogTemp,Display,TEXT("S47: balloon: the instrument box taken, day %d"),HandledDay);
 M->RequestAutosave();
 return true;
}
void FS47Balloon::Noted()
{
 // A record on OUT ON THE LAND, once, when lights were seen over this ground in the last two nights (FS47Happenings::StirredNear): the
 // plain thing that would account for them, kept as a lead like the land's others. Nothing is made up when nothing was seen.
 if(bNoted||!M||!M->Freeform||!M->Freeform->Places)return;
 auto& F=*M->Freeform;
 double X=0,Z=0;
 const bool bLights=F.Happenings&&F.Places->Spot(PlaceId,X,Z)&&F.Happenings->StirredNear(X,Z);
 bNoted=true;
 if(!bLights)return;
 F.RecordEvidence(FString::Printf(TEXT("balloon_lights_d%d"),F.Day()),LeadText(true));
 UE_LOG(LogTemp,Display,TEXT("S47: balloon: a lead on the land (lights were seen over that ground)"));
}
bool FS47Balloon::Mail()
{
 if(Box!=EBox::Carried||!M||!M->Freeform||!M->Freeform->Mail)return false;
 auto& F=*M->Freeform;
 FS47Mail::FPiece Reply;Reply.Id=TEXT("letter:balloon");Reply.Kind=TEXT("letter");Reply.Ref=TEXT("An envelope from the weather service, with a check");
 Reply.Text=ChequeText(F.DateOfDay(F.Day()).ToFormattedString(TEXT("%B %e, %Y")).ToUpper());Reply.Goods.Add(TEXT("dollars"),MailReward);
 if(!F.Mail->PostOut(TEXT("balloon"),TEXT("The balloon's instrument box, in its mailing bag, postage paid"),MailRounds,Reply))return false;
 Box=EBox::Mailed;HandledDay=F.Day();
 if(M->UI)M->UI->Toast(TEXT("The instrument box goes in the mailbox in its own mailing bag, postage paid, and the flag goes up for the carrier."),6);
 UE_LOG(LogTemp,Display,TEXT("S47: balloon: the instrument box left for the carrier, day %d"),HandledDay);
 M->RequestAutosave();
 return true;
}
bool FS47Balloon::CallSchool()
{
 if(Box!=EBox::Carried||!M||!M->Freeform||!M->UI)return false;
 auto& F=*M->Freeform;
 if(!SchoolOpen(F.Clock()))return false;
 SchoolClock=SchoolDue(F.Clock());Box=EBox::Promised;HandledDay=F.Day();
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Text(TEXT("The high school office. A long wait, a bell somewhere, then the science teacher, out of breath from the stairs.")));
 Rows.Add(S47Kit::Paper(TEXT("\"A whole radiosonde? Box, antenna, battery? You don't know what that's worth to us. The weather people send five dollars and a form letter. We send a boy with a pickup and ten dollars out of the club jar, and the kids get to take it apart.\n\nTomorrow afternoon, about three. Leave it at the station with whoever's there. Don't mail it. Promise me you won't mail it.\"")));
 Rows.Add(S47Kit::Small(FString::Printf(TEXT("They come for it tomorrow about 15:00. $%d, left at SARO, whether you are in or at the dishes."),SchoolReward)));
 Rows.Add(S47Kit::Button(TEXT("Put it down"),[this]{if(M&&M->UI)M->UI->Close();},true));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("CALICHE HIGH SCHOOL / OFFICE"),S47Kit::Col(Rows),760));
 F.RecordEvidence(TEXT("place_balloon_call"),TEXT("Called the high school office about the balloon's instrument box, as the Ledger's ad asked. The science teacher wants it whole; a boy with a pickup comes for it tomorrow afternoon, ten dollars from the club jar."));
 UE_LOG(LogTemp,Display,TEXT("S47: balloon: the school called, their pickup due at clock %.0f"),SchoolClock);
 M->RequestAutosave();
 return true;
}
FString FS47Balloon::TaskLine() const
{
 switch(Box)
 {
  case EBox::Carried:return TEXT("INSTRUMENT BOX / The balloon's box rides with your kit. Mail it from the box at the road (postage paid), or call the high school office from the operations line.");
  case EBox::Promised:return TEXT("INSTRUMENT BOX / The high school comes by SARO for the balloon's box tomorrow afternoon.");
  default:return FString();
 }
}
FString FS47Balloon::ChequeText(const FString& Date)
{
 return FString::Printf(TEXT("WEATHER SERVICE / UPPER AIR STATION\nBOX 2090, ALBUQUERQUE, NEW MEXICO\n%s\n\nTO THE FINDER, SARO, BOX 19, CALICHE\n\nThank you for returning the radiosonde instrument. It will be reconditioned and flown again. Enclosed is our check for $%d, the finder's fee for a complete instrument.\n\nInstruments of this kind are carried aloft on a balloon twice a day from stations across the country and come down by parachute within a hundred miles or so, where the wind takes them. Most are never seen again.\n\nUPPER AIR OBSERVATIONS"),*Date,MailReward);
}
FString FS47Balloon::LeadText(bool bLights)
{
 return bLights?TEXT("The balloon remains east of the highway, and lights seen over that ground these last nights. A thing like this goes up with a small battery lamp on it, drifts for hours at the edge of sight, and when the envelope bursts it comes down slow under its parachute: a light that moves without a sound and goes out low. It would account for what was seen, if you let it. It does not say it was that.")
  :TEXT("The balloon remains east of the highway. A thing like this goes up with a small battery lamp on it, drifts for hours and comes down slow under its parachute: a light that moves without a sound and goes out low.");
}
FString FS47Balloon::BoxRecord()
{
 return TEXT("Took the instrument box from the balloon remains east of the highway. A cardboard and foam box with a wire antenna and a battery pack, and a recovery lamp still blinking on it. The printed label asks the finder to mail it in, postage paid; the Ledger's ad says the high school will come and get it.");
}
bool FS47Balloon::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s balloon: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 // The beacon: on for a short blink about every 1.8 s, off between, faint by day.
 {
  bool bOn=true,bOff=true,bDay=true;int32 Blinks=0;bool bWas=false;
  for(double Tm=0;Tm<18;Tm+=.02){const bool bNow=Blink(Tm,true)>0;Blinks+=bNow&&!bWas?1:0;bWas=bNow;}
  bOn&=Blink(.05,true)==1.&&Blink(BlinkPeriod+.05,true)==1.;bOff&=Blink(.5,true)==0&&Blink(1.7,true)==0;
  bDay&=Blink(.05,false)>0&&Blink(.05,false)<=DayGlow&&Blink(.5,false)==0;
  Check(bOn&&bOff&&bDay&&Blinks==10&&Blink(-.1,true)==0&&Blink(NAN,true)==0,TEXT("the beacon blinks once every 1.8 s, briefly, bright at night and faint by day"));
 }
 // The school keeps office hours; its pickup comes the next afternoon at 15:00 (a call before 08:00: the same afternoon).
 Check(SchoolOpen(3*86400.+9*3600.)&&!SchoolOpen(3*86400.+16*3600.)&&!SchoolOpen(3*86400.+2*3600.)&&!SchoolOpen(NAN)
  &&SchoolDue(3*86400.+9*3600.)==4*86400.+15*3600.&&SchoolDue(3*86400.+15.5*3600.)==4*86400.+15*3600.&&SchoolDue(3*86400.+7*3600.)==3*86400.+15*3600.,
  TEXT("the office is open 08:00 to 16:00 and the pickup comes the next day at 15:00"));
 // The words: plain, no agency named, no spoiler, no 47; the letter says the fee; the lead leans to the plain reason and claims nothing.
 {
  const FString Letter=ChequeText(TEXT("MAY 12, 1986")),Lead=LeadText(true),Quiet=LeadText(false),Box=BoxRecord();
  const FString All=Letter+Lead+Quiet+Box;
  bool bPlain=S47Script::LintText(All).IsEmpty()&&!All.Contains(TEXT("47"))&&!All.ToLower().Contains(TEXT("alien"))&&!All.ToLower().Contains(TEXT("national weather"))&&!All.Contains(TEXT("NOAA"))&&!All.ToLower().Contains(TEXT("taken"));
  bPlain&=Letter.Contains(FString::Printf(TEXT("$%d"),MailReward))&&Letter.Contains(TEXT("MAY 12, 1986"))&&Lead.Contains(TEXT("would account for"))&&Lead.Contains(TEXT("does not say"))&&!Quiet.Contains(TEXT("these last nights"))&&Box.Contains(TEXT("postage paid"));
  Check(bPlain&&FS47FreeformCases::LineForFinding(TEXT("balloon_lights_d4"))==FS47FreeformCases::LandLine&&FS47FreeformCases::LineForFinding(TEXT("place_balloon_box"))==-1&&FS47FreeformCases::LineForFinding(TEXT("place_balloon_school"))==-1
   &&FS47FreeformSession::FieldRecordKey(TEXT("place_balloon_box")),
   TEXT("the letter, the lead and the records are plain and name no agency; the lead is a record on OUT ON THE LAND and the box itself a field record under no case"));
 }
 // The state through a save: lying writes nothing (an older save has none); carried, mailed, promised with its hour, gone come back.
 {
  FS47Balloon None(nullptr);None.Begin(nullptr);
  Check(None.State()==EBox::Lying&&None.Save()->Values.Num()==0&&None.TaskLine().IsEmpty()&&!None.Mail()&&!None.CallSchool(),TEXT("an older save has none of it: the box lies there, nothing to do with it yet, and writes nothing"));
  FS47Balloon A(nullptr);A.Begin(nullptr);A.Box=EBox::Promised;A.SchoolClock=5*86400.+15*3600.;A.HandledDay=4;A.bNoted=true;
  FS47Balloon B(nullptr);B.Begin(S47J::Read(S47J::Write(A.Save())));
  auto Odd=S47J::New();S47J::Set(Odd,TEXT("box"),TEXT("promised"));
  FS47Balloon C(nullptr);C.Begin(S47J::Read(S47J::Write(Odd)));
  auto Held=S47J::New();S47J::Set(Held,TEXT("box"),TEXT("carried"));
  FS47Balloon D(nullptr);D.Begin(S47J::Read(S47J::Write(Held)));
  Check(B.State()==EBox::Promised&&B.SchoolClock==A.SchoolClock&&B.HandledDay==4&&B.bNoted&&C.State()==EBox::Carried&&D.Carried()&&D.TaskLine().StartsWith(TEXT("INSTRUMENT BOX / "))&&B.TaskLine().Contains(TEXT("tomorrow afternoon"))
   &&S47Script::LintText(D.TaskLine()+B.TaskLine()).IsEmpty(),TEXT("the box's state, its day and the school's hour survive a Continue; a promise without an hour is the box still in hand; NOW says what to do with it"));
 }
 return OK;
}

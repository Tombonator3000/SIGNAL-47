#include "S47SmallHints.h"
#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47FieldRadio.h"
#include "S47Main.h"
#include "S47Taken.h"
#include "S47Freeform.h"
#include "S47Web.h"
#include "S47World.h"

namespace
{
 struct FHint{const TCHAR* Id;const TCHAR* Label;const TCHAR* Note;};
 const FHint Hints[]={
  {TEXT("phone_one_ring"),TEXT("Phone: one ring"),TEXT("the control room phone rings once and stops")},
  {TEXT("printer_line"),TEXT("Printer: a line by itself"),TEXT("the printer runs for two seconds; nothing you asked for")},
  {TEXT("rack_click"),TEXT("Rack: relays click"),TEXT("three clicks from the rack, with no one at it")},
  {TEXT("radio_crackle"),TEXT("Radio: a crackle"),TEXT("the set, or the rack's monitor, opens for a moment")},
  {TEXT("service_door_open"),TEXT("Service door found open"),TEXT("it was shut; it stands open when you are out of sight of it")},
  {TEXT("antenna_creak"),TEXT("An antenna creaks"),TEXT("metal, slow, from among the dishes, with no wind to speak of")},
  {TEXT("fence_tap"),TEXT("Taps along the fence"),TEXT("metal on metal, a few, moving off")},
  {TEXT("night_hush"),TEXT("The crickets stop"),TEXT("half a minute of nothing outside, then they come back")},
 };
 const FHint* Find(const FString& Id){for(const FHint& H:Hints)if(Id==H.Id)return &H;return nullptr;}
 const FVector Rack(-3,1,1),Printer(-4,1,2),FenceAt(11.9,1,-11),AntennaAt(-3.25,3,-47);
 const TCHAR* const ServiceDoor=TEXT("doorEast");
 bool Near(const FVector& A,const FVector& B,double Range){return FVector2D::Distance(FVector2D(A.X,A.Z),FVector2D(B.X,B.Z))<=Range;}
 bool OutOfDoors(US47Main* M){return M&&M->Freeform&&M->Freeform->Taken&&!M->Freeform->Taken->Indoors();}
}

bool S47SmallHints::Is(const FString& Id){return Find(Id)!=nullptr;}
TArray<FString> S47SmallHints::Ids(){TArray<FString> Out;for(const FHint& H:Hints)Out.Add(H.Id);return Out;}
const TCHAR* S47SmallHints::TitleOf(const FString& Id){const FHint* H=Find(Id);return H?H->Label:TEXT("");}
const TCHAR* S47SmallHints::NoteOf(const FString& Id){const FHint* H=Find(Id);return H?H->Note:TEXT("");}
bool S47SmallHints::Anywhere(const FString& Id){return Id==TEXT("night_hush")||Id==TEXT("radio_crackle");}

FVector S47SmallHints::AnchorOf(US47Main* M,const FString& Id)
{
 if(Id==TEXT("phone_one_ring")&&M&&M->Web)
 {
  const FS47Obj Phone=M->Web->Obj(TEXT("room.objs.phone"));
  if(Phone)return Phone.WorldPosition();
  return Rack;
 }
 if(Id==TEXT("printer_line"))return Printer;
 if(Id==TEXT("rack_click")||Id==TEXT("radio_crackle"))return Rack;
 if(Id==TEXT("service_door_open"))return FVector(9,1,0);
 if(Id==TEXT("antenna_creak"))return AntennaAt;
 if(Id==TEXT("fence_tap"))return FenceAt;
 return Rack;
}

bool S47SmallHints::Fits(US47Main* M,const FString& Id,const FVector& PlayerAt,TFunctionRef<bool(const FVector&)> InView)
{
 if(!M||!M->Web)return false;
 const FVector A=AnchorOf(M,Id);
 if(Id==TEXT("phone_one_ring"))return Near(PlayerAt,A,45);
 if(Id==TEXT("printer_line")||Id==TEXT("rack_click"))return Near(PlayerAt,A,22);
 if(Id==TEXT("radio_crackle"))return Near(PlayerAt,A,22)||(M->FieldRadio&&M->FieldRadio->Owned());
 if(Id==TEXT("service_door_open"))return !M->Web->IsDoorOpen(ServiceDoor)&&!Near(PlayerAt,A,25)&&!InView(A);
 if(Id==TEXT("antenna_creak"))return OutOfDoors(M)&&Near(PlayerAt,A,70);
 if(Id==TEXT("fence_tap"))return OutOfDoors(M)&&Near(PlayerAt,A,45);
 if(Id==TEXT("night_hush"))return OutOfDoors(M);
 return false;
}

void S47SmallHints::Play(US47Main* M,const FString& Id,const FVector& Anchor)
{
 if(!M||!M->Audio)return;
 US47Audio& A=*M->Audio;
 if(Id==TEXT("phone_one_ring"))
 {
  // As Story's own (FS47NightEvents): one ring, cut off.
  FS47Sound O;O.At=Anchor;
  const FS47Play H=A.Sfx(TEXT("phoneRing"),O);
  if(H&&A.Ctx.IsValid()){const double T=A.Ctx->CurrentTime();H.Gain->Gain.SetValueAtTime(1,T+1.7);H.Gain->Gain.LinearRampToValueAtTime(0,T+1.85);H.Src->Stop(T+1.9);}
 }
 else if(Id==TEXT("printer_line"))
 {
  FS47Sound O;O.At=Anchor;O.Gain=.3f;
  const FS47Play H=A.Sfx(TEXT("printer"),O);
  if(H&&A.Ctx.IsValid())H.Src->Stop(A.Ctx->CurrentTime()+2.1);
 }
 else if(Id==TEXT("rack_click"))
 {
  for(int32 I=0;I<3;++I){FS47Sound O;O.At=Anchor;O.Gain=.4f-.08f*I;O.When=.45f*I+FMath::FRand()*.2f;A.Sfx(TEXT("rack_switch"),O);}
 }
 else if(Id==TEXT("radio_crackle"))
 {
  FS47Sound O;O.Gain=.35f;O.Dest=TEXT("radio");
  if(!(M->FieldRadio&&M->FieldRadio->Owned()))O.At=Anchor;
  A.Sfx(A.Or(TEXT("radio_crackle_short"),TEXT("alienRadio")),O);
 }
 else if(Id==TEXT("service_door_open"))
 {
  if(M->Web)M->Web->SetDoorById(ServiceDoor,true,false);
 }
 else if(Id==TEXT("antenna_creak"))
 {
  FS47Sound O;O.At=Anchor;O.Gain=.4f;O.Rate=.45f;O.Lp=1400;
  A.Sfx(TEXT("doorCreak1"),O);
 }
 else if(Id==TEXT("fence_tap"))
 {
  for(int32 I=0;I<4;++I){FS47Sound O;O.At=Anchor+FVector(I*1.5,0,0);O.Gain=.3f-.05f*I;O.When=.8f*I+FMath::FRand()*.4f;O.Lp=2600;A.Sfx(TEXT("metalTap"),O);}
 }
 else if(Id==TEXT("night_hush"))
 {
  if(A.Night)A.Night->Hush(30,.05,.8,3.);
 }
}

bool S47SmallHints::PaceAllows(int32 Spent,int32 Budget,double Hour,double Jitter)
{
 if(Budget<=0||Spent>=Budget)return false;
 // Through the dark, 19 to 06: what is spent stays near how far the night has gone, so that small hours are left for it.
 const double Night=Hour>=19?Hour-19:Hour<6?Hour+5:0.;
 const double Frac=FMath::Clamp(Night/11.,0.,1.);
 return Spent<FMath::CeilToInt(Budget*Frac+Jitter*.75);
}

bool S47SmallHints::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s small hints: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 Check(Ids().Num()==UE_ARRAY_COUNT(Hints)&&Is(TEXT("night_hush"))&&!Is(TEXT("flyover")),TEXT("the list knows its own"));
 Check(PaceAllows(0,3,19.5,1)&&!PaceAllows(0,3,19.,0),TEXT("with a good draw the first can come early, and not at nineteen with a bad one"));
 Check(!PaceAllows(2,3,20,.5),TEXT("the third of three is not spent in the first hour"));
 Check(PaceAllows(2,3,2.,0),TEXT("but by two in the morning it may be"));
 Check(!PaceAllows(3,3,5,1),TEXT("never past the budget"));
 return OK;
}

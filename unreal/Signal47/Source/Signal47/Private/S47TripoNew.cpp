#include "S47TripoNew.h"
#include "S47EyeGlint.h"
#include "Misc/PackageName.h"

namespace S47TripoNewLocal
{
 const TCHAR* const Cube=TEXT("/Engine/BasicShapes/Cube.Cube");
 const TCHAR* const TallStandIn=TEXT("/Game/Licensed/TripoNPC/SM_Tripo_tall_one.SM_Tripo_tall_one");
}

const TArray<S47TripoNew::FModel>& S47TripoNew::Models()
{
 using S47TripoNewLocal::Cube;using S47TripoNewLocal::TallStandIn;
 // Sizes as Tools/triponew/prepare.py makes them. The stand-ins are S47Happenings' own where it has one.
 static const TArray<FModel> All={
  {TEXT("singer"),TEXT("Big man in a white suit (never named)"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Singer.SM_Tripo_Singer"),
   TEXT("/Game/Licensed/TripoNPC/SM_Tripo_souvenir_man.SM_Tripo_souvenir_man"),EKind::Figure,1.83,0xb9b2a4u},
  {TEXT("bigfoot"),TEXT("Something tall and hairy (never named)"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Bigfoot.SM_Tripo_Bigfoot"),
   TEXT("/Game/Licensed/TripoNPC/SM_Tripo_shadow_man.SM_Tripo_shadow_man"),EKind::Figure,2.45,0x3d2b1cu},
  {TEXT("zet"),TEXT("Zet: small, gray, shirt and hat"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Zet.SM_Tripo_Zet"),Cube,EKind::Alien,1.40,0x8c938du},
  {TEXT("nordic"),TEXT("Nordic: tall, pale, fair hair"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Nordic.SM_Tripo_Nordic"),TallStandIn,EKind::Alien,1.96,0xc8c8c8u},
  {TEXT("cosmic"),TEXT("Cosmic: very tall, thin, blue-green"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Cosmic.SM_Tripo_Cosmic"),TallStandIn,EKind::Alien,2.30,0x2f7a78u},
  {TEXT("motorhome"),TEXT("Old motorhome, cream and brown"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Motorhome.SM_Tripo_Motorhome"),Cube,EKind::Vehicle,7.3,0xd8ccb0u},
  {TEXT("convertible"),TEXT("Pink convertible with fins"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_PinkConvertible.SM_Tripo_PinkConvertible"),
   TEXT("/Game/Free/Vehicles/el_alamo_78/StaticMeshes/el_alamo_78.el_alamo_78"),EKind::Vehicle,5.7,0xe58fb4u},
  {TEXT("walkie"),TEXT("Walkie-talkie with a display and keypad (too new for 1986, not used)"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Walkie.SM_Tripo_Walkie"),Cube,EKind::Prop,.34,0x1c1d1cu},
  // Named a parcel on Tom's desktop ("pakke tripo3d-1"), but an eighties walkie-talkie: the one on the SARO desk (DeskWalkie).
  {TEXT("parcel"),TEXT("Eighties walkie-talkie (Tom's \"pakke\"), the desk walkie"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Parcel.SM_Tripo_Parcel"),Cube,EKind::Prop,.31,0x6b6d6au},
 };
 return All;
}

const S47TripoNew::FModel* S47TripoNew::Find(const FString& Id)
{
 for(const FModel& M:Models())if(Id==M.Id)return &M;
 return nullptr;
}

const S47TripoNew::FModel* S47TripoNew::DeskWalkie(){return Find(TEXT("parcel"));}

bool S47TripoNew::Exists(const TCHAR* ObjectPath)
{
 return ObjectPath&&*ObjectPath&&FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath)));
}

const TCHAR* S47TripoNew::Resolve(const FModel& Model,bool* bOwn)
{
 const bool bHave=Exists(Model.Path);
 if(bOwn)*bOwn=bHave;
 if(bHave)return Model.Path;
 return Exists(Model.StandIn)?Model.StandIn:S47TripoNewLocal::Cube;
}

bool S47TripoNew::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto Check=[&](bool Pass,const TCHAR* Text){Lines.Add(FString(Pass?TEXT("PASS "):TEXT("FAIL "))+Text);OK&=Pass;};
 bool bPrivate=true,bNamed=true,bSized=true;
 for(const FModel& M:Models())
 {
  const FString P(M.Path);
  bPrivate&=P.StartsWith(TEXT("/Game/Licensed/TripoNew/SM_Tripo_"));
  const FString Asset=FPackageName::ObjectPathToObjectName(P);
  bNamed&=!Asset.IsEmpty()&&P.EndsWith(TEXT("/")+Asset+TEXT(".")+Asset);
  bSized&=M.SizeM>.1&&M.SizeM<10.;
 }
 Check(Models().Num()==9&&bPrivate,TEXT("Tripo new: the nine models, every one under /Game/Licensed/TripoNew (out of Git)"));
 Check(bNamed&&bSized,TEXT("Tripo new: each path names its own asset, and each has a size"));
 // S47Happenings waits for these five by name (S47Hap::RvModel ...): the same names here.
 Check(Find(TEXT("singer"))&&FString(Find(TEXT("singer"))->Path).EndsWith(TEXT("SM_Tripo_Singer"))&&FString(Find(TEXT("bigfoot"))->Path).EndsWith(TEXT("SM_Tripo_Bigfoot"))
  &&FString(Find(TEXT("zet"))->Path).EndsWith(TEXT("SM_Tripo_Zet"))&&FString(Find(TEXT("motorhome"))->Path).EndsWith(TEXT("SM_Tripo_Motorhome"))
  &&FString(Find(TEXT("convertible"))->Path).EndsWith(TEXT("SM_Tripo_PinkConvertible")),TEXT("Tripo new: the humor events' five paths are the ones imported"));
 // Tom, 9 November: the "parcel" is an eighties walkie-talkie, and it is the one on the desk; the newer-looking one is not.
 Check(DeskWalkie()&&FString(DeskWalkie()->Path).EndsWith(TEXT("SM_Tripo_Parcel"))&&DeskWalkie()->Kind==EKind::Prop&&DeskWalkieHeightM>.2&&DeskWalkieHeightM<.45,
  TEXT("Tripo new: the desk walkie is Tom's eighties one (SM_Tripo_Parcel), at a hand-held height"));
 // Without the private models (a build from Git alone) each still resolves to something drawable.
 FModel Missing=Models()[0];Missing.Path=TEXT("/Game/Licensed/TripoNew/SM_Tripo_NotThere.SM_Tripo_NotThere");Missing.StandIn=TEXT("/Game/Nowhere/SM_Gone.SM_Gone");
 bool bOwn=true;const TCHAR* Got=Resolve(Missing,&bOwn);
 Check(!bOwn&&Got&&FString(Got)==TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("Tripo new: a missing model and a missing stand-in fall back to a basic shape, not to nothing"));
 bool bAll=true;for(const FModel& M:Models())bAll&=Resolve(M)!=nullptr;
 Check(bAll,TEXT("Tripo new: every model resolves to something in this build"));
 // Zet's big black eyes have the dot (S47EyeGlint::ForStatic): either side of the middle, ahead of the pivot, up in the head.
 const S47EyeGlint::FEyes* Zet=S47EyeGlint::ForStatic(TEXT("SM_Tripo_Zet"));
 const double ZetCm=Find(TEXT("zet"))->SizeM*100.;
 Check(Zet&&Zet->Left.X>2.&&Zet->Right.X<-2.&&Zet->Left.Y>5.&&Zet->Right.Y>5.&&Zet->Left.Z>ZetCm*.75&&Zet->Left.Z<ZetCm&&FMath::Abs(Zet->Left.Z-Zet->Right.Z)<3.,
  TEXT("Tripo new: Zet's eyes measured for the glint, in the head, one each side"));
 // The Nordic's eyes are a man's and the Cosmic one has none: no dot on either, nor on the singer or the hairy one.
 Check(!S47EyeGlint::ForStatic(TEXT("SM_Tripo_Nordic"))&&!S47EyeGlint::ForStatic(TEXT("SM_Tripo_Cosmic"))&&!S47EyeGlint::ForStatic(TEXT("SM_Tripo_Singer"))
  &&!S47EyeGlint::ForStatic(TEXT("SM_Tripo_Bigfoot")),TEXT("Tripo new: no glint where there is no black eye (Nordic, Cosmic, the singer, the hairy one)"));
 return OK;
}

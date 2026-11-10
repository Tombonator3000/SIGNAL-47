#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;
class AS47Web;
class UStaticMesh;
class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

// D48-20261110 (Tom, after d46: the balloon find "needs a model; a bit more out of the balloon event; do you get anything for it?").
// Freeform only. The balloon remains east of the highway (FS47Places "balloon"), drawn from the four pieces Tools/balloon/import_assets.sh
// brings in (/Game/Licensed/Models/balloon/StaticMeshes: envelope, parachute, cord, radiosonde), seated on the ground where the place
// stands; where the models are not, the basic shapes stand for the rubber, the chute and the cord (FS47Places::PartsOf) and a pale box
// for the instrument. The box is always this class's: on it a small battery recovery beacon, a red lamp that blinks about every 1.8 s
// (an emissive pulse and a low point light with no shadows), so the find can be seen from a few hundred metres at night (Tom: easier to
// find); by day it is faint. It goes out when the box is picked up.
// Picked up (E on the box), the box rides with the player like the kit does, and then one of two things:
//   mailed    left in the SARO mailbox for the carrier (FS47Mail::PostOut; the flag goes up as for an order): a few rounds later an
//             envelope comes back with a $5 check, the weather service's finder's fee (the return label asks for the instrument,
//             postage paid), cashed to the budget when it is taken out of the box;
//   called    the high school office, from the operations line (the Ledger's WANTED ad): they come for it the next afternoon, $10.
// Taken while lights were seen out on that ground in the last two nights (FS47Happenings::StirredNear), the find is a record on OUT ON
// THE LAND (the lead "balloon_lights_d<day>", FS47FreeformCases::LineForFinding), in plain words: a thing that drifts for hours with a
// small light on it and comes down slow would account for a light seen over the valley, if you let it. It is a lead like the land's
// others, never a test result and never proof: the case rules stand (a filing on any evidence; the controls use the kit).
// Saved under "balloon" (state, the day it was handled, the school's clock); an older save has none and the box lies where it was.
class FS47Balloon
{
public:
 explicit FS47Balloon(US47Main* Main):M(Main){}
 enum class EBox:uint8{Lying,Carried,Mailed,Promised,Gone};
 static constexpr const TCHAR* Folder=TEXT("/Game/Licensed/Models/balloon/StaticMeshes/");
 static constexpr const TCHAR* PlaceId=TEXT("balloon");
 // Every piece of the model is in the project (the envelope, the parachute, the cord and the radiosonde): then FS47Places draws no
 // basic shapes for the place and this class draws it all.
 static bool HaveModel();
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 EBox State() const{return Box;}
 bool Carried() const{return Box==EBox::Carried;}
 bool Lying() const{return Box==EBox::Lying;}
 // E on the box: into the hand and along with the player. False: it is not lying there.
 bool Take();
 // Left in the mailbox for the carrier (FS47Mail::OpenBox has the button). False: nothing carried, or no post in this game.
 bool Mail();
 // The high school office called from the operations line: they come for it tomorrow afternoon. False: nothing carried, or closed.
 bool CallSchool();
 // The office keeps school hours: 08:00 to 16:00 by the world clock. Pure.
 static bool SchoolOpen(double Clock);
 // When the school's pickup comes for it: the next day at 15:00 (a call before 08:00 the same day: that afternoon). Pure.
 static double SchoolDue(double Clock);
 // The task list's row (FS47FreeformSession::Tasks); empty when there is none.
 FString TaskLine() const;
 // The beacon by the clock of its own cycle: 1 for the blink, 0 between (BlinkOn seconds of every BlinkPeriod), and by day (bNight false)
 // a faint one at most. Pure.
 static double Blink(double T,bool bNight);
 static constexpr double BlinkPeriod=1.8,BlinkOn=.14,DayGlow=.12;
 static constexpr int32 MailReward=5,SchoolReward=10,MailRounds=3;
 // The letter that comes back with the check, and the words of the records kept. Pure, plain, no agency named.
 static FString ChequeText(const FString& Date);
 static FString LeadText(bool bLights);
 static FString BoxRecord();
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 EBox Box=EBox::Lying;
 int32 HandledDay=0;
 double SchoolClock=-1;
 bool bBuilt=false,bModel=false,bSeated=false,bLit=false,bNoted=false;
 double T=0,SeatIn=0;
 FVector Stand=FVector::ZeroVector; // three-space world, metres, the place's own ground
 TSharedPtr<uint8> Life;
 TWeakObjectPtr<AS47Web> Scene;
 // The pieces that stay (the envelope, the chute, the cord) and the box with its lamp; each part's offset from the place, metres.
 struct FPart{TWeakObjectPtr<UStaticMeshComponent> C;FVector At;bool bBox=false,bLamp=false;};
 FS47Obj BoxObj;
 TArray<FPart> Parts;
 TWeakObjectPtr<UPointLightComponent> Lamp;
 TWeakObjectPtr<UMaterialInstanceDynamic> Glow;
 FVector LampAt=FVector::ZeroVector;
 bool Build();
 void Seat(bool bForce);
 void Show();
 void Noted();
 UStaticMesh* Piece(const TCHAR* Name) const;
 UStaticMeshComponent* Part(UStaticMesh* Mesh,const FString& Name,const FVector& At,const FVector& Scale,float Yaw,UMaterialInterface* Mat,bool bBox);
};

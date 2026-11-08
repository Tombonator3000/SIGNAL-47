#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Walker.h"

class US47Main;
class SS47AwayOverlay;
class USpotLightComponent;
class UPointLightComponent;
class FS47OutsideLight;
#include "S47UfoPresentation.h"

// Who takes the player decides how: how long is gone, and where they come to. Different bodies are different in kind,
// not stronger versions of one thing.
enum class ES47Taker : uint8 { Roswell, Crab, Tall };

struct FS47TakenTerms
{
 double MinMinutes=0,MaxMinutes=0;
 bool bLift=true;       // lifted in the light; else dark comes at once
 double WakeMin=0,WakeMax=0; // metres from where they stood; 0: the same place
 bool bHome=false;      // comes to at SARO with the truck on its pad
 FLinearColor Light=FLinearColor::White;
 int32 Leaves=0;        // how much of what they carry stays behind on the ground: the set, then the camera
};

// A flood of light into the place, and being taken in it (GAME_BIBLE, Freeform-bortføring og tapt tid): a warning that
// can be read, a way out of the light, the clock moved once, everything secured kept, and calm afterwards.
class FS47Taken
{
public:
 explicit FS47Taken(US47Main* Main):M(Main){}
 ~FS47Taken();
 void Stop();
 void Step(double Dt);
 enum class EStage : uint8 { None, Dread, Flood, Lift, Away };
 EStage Stage() const{return Now;}
 bool Busy() const{return Now!=EStage::None;}
 // From the first sign. Under a roof it begins overhead; in the open the light comes straight away.
 bool Begin(ES47Taker Who,const FString& Title,bool bOnlyDread=false);
 // In the open, for someone on foot: the light stands over that place, faint, long enough to read and to leave, and only
 // then starts walking toward them. The same way out as ever: distance, a wall, or waiting it out.
 bool BeginOver(ES47Taker Who,const FString& Title,const FVector& Over);
 static constexpr double OverWarnSeconds=8;
 // Reached by a body: past the warning.
 bool TakeNow(ES47Taker Who,const FString& Title);
 void GiveUp();
 bool Indoors() const;
 double LightOn() const{return InLight;}
 // Where a radio breaks up on it while it is coming: over the roof, then where the light is.
 TOptional<FVector> RadioAt() const{return Now==EStage::Dread?TOptional<FVector>(StepAt):Now==EStage::Flood?TOptional<FVector>(Aim):TOptional<FVector>();}
 FVector LightFrom() const{return From;}
 const FString& LastNote() const{return Note;}
 static const FS47TakenTerms& Terms(ES47Taker Who);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 EStage Now=EStage::None;
 ES47Taker Taker=ES47Taker::Roswell;
 FString Place,Note;
 double Age=0,StepIn=0,TalkIn=0,InLight=0,Seconds=0;
 double SafeClock=0;
 FVector From=FVector::ZeroVector,Aim=FVector::ZeroVector,StartEye=FVector::ZeroVector;
 FVector StepAt=FVector::ZeroVector;
 // Where and in what area the light first found them, for the room that remembers it.
 FVector FloodFeet=FVector::ZeroVector;
 FString FloodArea;
 double HushIn=0;
 bool bOutdoors=false,bHooked=false,bOverlay=false,bSpill=false,bDoorDone=false,bOverPlace=false;
 double StartYaw=0,StartPitch=0;
 TSharedPtr<SS47AwayOverlay> Overlay;
 TWeakObjectPtr<USpotLightComponent> Spot;
 TWeakObjectPtr<UPointLightComponent> Fill;
 // Under a roof the light is the massive one from outside (S47OutsideLight.h). The air is its own too, here and in the open.
 TSharedPtr<FS47OutsideLight> Rig;
 void StepMist(double Dt);
 double MistK=0;
 FLinearColor LightColor(double Time) const;
 double Pulse(double Time) const;
 TUniquePtr<FS47UfoPresentation> Ufo;
 TFunction<bool(double,const FVector2D&,const FVector2D&,FS47View&)> BeforeCamera;
 TWeakObjectPtr<AS47Walker> Hooked;
 TSharedPtr<bool> Alive=MakeShared<bool>(true);
 bool StartFlood();
 bool ChooseLight();
 void Lift();
 void Unhook();
 void Finish();
 void Apply();
 void ShowLight(double K);
 void HideLight();
 void White(TOptional<double> K);
 bool LightReaches(const FVector& Chest) const;
 FVector Feet() const;
};

#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

// Retained first-hand facts only. No hidden cause, live target or conclusion.
struct FS47FreeformObservation
{
 FString Id,Event,Place,Bearing,PhotoId;
 int32 Day=0;
 double Clock=0; // absolute Freeform seconds, not wall time or completion time
 bool bHeard=false;
 static FString Direction(const FVector& From,const FVector& At)
 {
  const FVector D=At-From;
  if(FVector2D(D.X,D.Z).Size()<.5)return TEXT("overhead / local");
  const double Degrees=FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(D.X,-D.Z))+360.,360.);
  static const TCHAR* Names[]={TEXT("N"),TEXT("NE"),TEXT("E"),TEXT("SE"),TEXT("S"),TEXT("SW"),TEXT("W"),TEXT("NW")};
  return Names[int32(FMath::FloorToDouble((Degrees+22.5)/45.))%8];
 }
 bool Valid() const
 {
  static const TSet<FString> Directions={TEXT("N"),TEXT("NE"),TEXT("E"),TEXT("SE"),TEXT("S"),TEXT("SW"),TEXT("W"),TEXT("NW"),TEXT("overhead / local")};
  return !Id.IsEmpty()&&Id.Len()<=160&&!Event.IsEmpty()&&Event.Len()<=64&&!Place.IsEmpty()&&Place.Len()<=160&&Directions.Contains(Bearing)&&PhotoId.Len()<=160&&Day>0&&Day<=1000000&&FMath::IsFinite(Clock)&&Clock>=0&&Clock<=86400000000.;
 }
 S47J::FObj Save() const
 {
  auto O=S47J::New();S47J::Set(O,TEXT("id"),Id);S47J::Set(O,TEXT("event"),Event);S47J::Set(O,TEXT("place"),Place);S47J::Set(O,TEXT("bearing"),Bearing);S47J::Set(O,TEXT("photo"),PhotoId);S47J::Set(O,TEXT("day"),Day);S47J::Set(O,TEXT("clock"),Clock);S47J::Set(O,TEXT("heard"),bHeard);return O;
 }
 static bool Read(const S47J::FObj& O,FS47FreeformObservation& Out)
 {
  if(!O)return false;
  FS47FreeformObservation V;V.Id=S47J::Str(O,TEXT("id"));V.Event=S47J::Str(O,TEXT("event"));V.Place=S47J::Str(O,TEXT("place"));V.Bearing=S47J::Str(O,TEXT("bearing"));V.PhotoId=S47J::Str(O,TEXT("photo"));V.Day=S47J::Int(O,TEXT("day"));V.Clock=S47J::Num(O,TEXT("clock"),-1);V.bHeard=S47J::Bool(O,TEXT("heard"));if(!V.Valid())return false;Out=V;return true;
 }
 FString Text() const
 {
  const int32 Seconds=int32(FMath::Fmod(Clock,86400.));
  return FString::Printf(TEXT("FIELD OBSERVATION / %s\nDAY %d / %02d:%02d:%02d\n%s / approximate bearing %s\nSource: %s (%s).\nPhotograph: %s\nThis is an observation, not a confirmed identity or shared cause."),*Id,Day,Seconds/3600,(Seconds/60)%60,Seconds%60,*Place,*Bearing,*Event,bHeard?TEXT("heard; no visual identity"):TEXT("seen"),PhotoId.IsEmpty()?TEXT("none retained"):*PhotoId);
 }
};

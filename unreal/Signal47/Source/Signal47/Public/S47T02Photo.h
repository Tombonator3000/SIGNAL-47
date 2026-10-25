#pragma once

#include "CoreMinimal.h"
#include "S47FieldCamera.h"
#include "S47Json.h"

class US47Main;

// One reserved instant-back exposure. Substation owns the value and attempt lifecycle.
// ToJson is deliberately flat: id/url are direct members of substation.t02.photo,
// within FS47SaveStore's depth-six photo traversal. Never nest this under another photo object.
struct SIGNAL47_API FS47T02PhotoRecord
{
 FS47Photo Photo;
 FString AttemptId;
 FVector SourcePosition = FVector::ZeroVector; // exact Walker::Pos, Three metres
 FVector SourceEye = FVector::ZeroVector; // actual scene-capture eye, Three metres
 double SourceYaw = 0, SourcePitch = 0, SourceFloorY = 0;
 FRotator CameraRotation = FRotator::ZeroRotator; // actual scene-capture rotation, UE degrees
 TArray<TPair<FString, FVector>> Features; // world Three positions at exposure
 bool bUnavailable = false;

 S47J::FObj ToJson() const;
 static TOptional<FS47T02PhotoRecord> FromJson(const S47J::FObj& O);
 bool Valid(const FString& ExpectedAttemptId, const FVector& Reference, double Radius,
  const TArray<TPair<FString, FVector>>& RequiredFeatures) const;

private:
 FString ImageKey;
 FString DecodedUrl;
 bool bImageDecoded = false;
 friend struct FS47T02Photo;
};

// A narrow Chapter1 camera dispatcher, before Field/Free. It never owns either roll,
// modifies chapter-three lab hooks, sets the reference flag or fabricates a print.
struct SIGNAL47_API FS47T02Photo
{
 static bool InContext(US47Main* M);
 // Geometry only, also usable by the physical reference-marker interaction before controls.
 // Camera need not be raised here; Sync still projects the actual eye through the photo lens.
 static bool CheckView(US47Main* M, FString& Reason);
 // Current controller prerequisites and the same physical view, for the viewfinder.
 static bool Ready(US47Main* M, FString& Reason);
 // True means this near-T02 shutter was handled, including a refusal with no film consumed.
 static bool TryShutter(US47Main* M);
};

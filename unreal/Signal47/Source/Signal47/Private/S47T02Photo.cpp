#include "S47T02Photo.h"

#include "S47Audio.h"
#include "S47Image.h"
#include "S47Main.h"
#include "S47Saves.h"
#include "S47Story.h"
#include "S47Substation.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/SpotLightComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47T02Photo, Log, All);

namespace
{
 constexpr double Edge = .04;

 bool Finite(const FVector& P)
 {
  return FMath::IsFinite(P.X) && FMath::IsFinite(P.Y) && FMath::IsFinite(P.Z);
 }
 bool FiniteRotation(const FRotator& R)
 {
  return FMath::IsFinite(R.Pitch) && FMath::IsFinite(R.Yaw) && FMath::IsFinite(R.Roll);
 }
 bool ReadVector(const S47J::FObj& O, const TCHAR* Key, FVector& P)
 {
  const auto A = S47J::Arr(O, Key);
  double X = 0, Y = 0, Z = 0;
  if (A.Num() != 3 || !A[0].IsValid() || !A[1].IsValid() || !A[2].IsValid() ||
   !A[0]->TryGetNumber(X) || !A[1]->TryGetNumber(Y) || !A[2]->TryGetNumber(Z)) return false;
  P = FVector(X, Y, Z); return Finite(P);
 }
 void WriteVector(const S47J::FObj& O, const TCHAR* Key, const FVector& P)
 {
  S47J::Set(O, Key, TArray<double>{P.X, P.Y, P.Z});
 }
 FS47Projection ProjectAt(const FS47T02PhotoRecord& R, const FVector& P)
 {
  const FVector L = R.CameraRotation.UnrotateVector(S47Three::ToUE(P) - S47Three::ToUE(R.SourceEye));
  FS47Projection Out;
  if (L.X <= .001) return Out;
  const double TanV = FMath::Tan(US47FieldCamera::PHOTO_FOV * PI / 360);
  Out.U = ((L.Y / L.X) / (TanV * US47FieldCamera::PHOTO_W / US47FieldCamera::PHOTO_H) + 1) / 2;
  Out.V = (1 - (L.Z / L.X) / TanV) / 2;
  Out.bFront = true; return Out;
 }
 bool Inside(const FS47Projection& P)
 {
  return P.bFront && FMath::IsFinite(P.U) && FMath::IsFinite(P.V) &&
   P.U >= Edge && P.U <= 1 - Edge && P.V >= Edge && P.V <= 1 - Edge;
 }
 bool DecodePrint(const FString& Url)
 {
  if (!Url.StartsWith(TEXT("data:image/jpeg;base64,")) || Url.Len() > 4 * 1024 * 1024) return false;
  TArray<FColor> Pixels; int32 Width = 0, Height = 0;
  return S47Image::DecodePixels(Url, Pixels, Width, Height) &&
   Width == US47FieldCamera::PRINT_W && Height == US47FieldCamera::PRINT_H && Pixels.Num() == Width * Height;
 }
 bool Geometry(US47Main* M, FString& Reason, FS47T02PhotoRecord& Out)
 {
  if (!FS47T02Photo::InContext(M) || !M->FieldCamera || !M->FieldCamera->bHave || !M->Player || !M->Web)
  { Reason = TEXT("USE THE FIELD CAMERA AT THE MARKED CALICHE REFERENCE POSITION"); return false; }
  if (M->Player->InAir())
  { Reason = TEXT("STAND STILL ON THE CALICHE REFERENCE POSITION"); return false; }
  const FVector Ref = M->Substation->FollowupReferencePosition();
  const double Radius = M->Substation->FollowupReferenceRadius();
  const FVector Position = M->Player->Pos;
  if (!Finite(Position) || !Finite(Ref) || !FMath::IsFinite(Radius) || Radius <= 0 ||
   FVector2D(Position.X - Ref.X, Position.Z - Ref.Z).Size() > Radius)
  { Reason = TEXT("RETURN TO THE MARKED CALICHE REFERENCE POSITION"); return false; }
  US47FieldCamera* Camera = M->FieldCamera;
  Camera->Sync();
  Out.SourcePosition = Position;
  Out.SourceEye = S47Three::ToThree(Camera->PhotoLoc);
  Out.SourceYaw = M->Player->Yaw; Out.SourcePitch = M->Player->Pitch;
  Out.SourceFloorY = M->Player->FloorY; Out.CameraRotation = Camera->PhotoRot;
  Out.AttemptId = M->Substation->FollowupAttemptId();
  Out.Features = M->Substation->FollowupFeatures();
  const TArray<FS47Obj> Occluders = M->Substation->FollowupOccluders();
  if (!Finite(Out.SourceEye) || !FiniteRotation(Out.CameraRotation) ||
   !FMath::IsFinite(Out.SourceFloorY) || FMath::Abs(Out.SourceFloorY - Ref.Y) > 1 ||
   Out.Features.Num() < 3 || Occluders.IsEmpty())
  { Reason = TEXT("THE PHYSICAL CALICHE REFERENCE IS NOT AVAILABLE"); return false; }
  TSet<FString> Seen;
  for (const auto& Feature : Out.Features)
  {
   if (Feature.Key.IsEmpty() || Seen.Contains(Feature.Key) || !Finite(Feature.Value))
   { Reason = TEXT("THE PHYSICAL CALICHE REFERENCE IS NOT AVAILABLE"); return false; }
   Seen.Add(Feature.Key);
   const FS47Projection P = Camera->Project(Feature.Value);
   if (!Inside(P) || !Camera->InFrame(Feature.Value, Edge, 1 - Edge, Edge, 1 - Edge))
   { Reason = TEXT("FRAME THE CULVERT, WHITE BANDS, OUTER CONDUCTORS, FENCE AND LAMP TOGETHER"); return false; }
   if (!Camera->ClearLine(Feature.Value, Occluders))
   { Reason = TEXT("REFERENCE OBSTRUCTED // FIND THE CLEAR MARKED SIGHT LINE"); return false; }
   Out.Photo.Targets.Add({Feature.Key, FVector2D(P.U, P.V)});
  }
  Reason = TEXT("CALICHE REFERENCE IN FRAME // RESERVED INSTANT PRINT READY"); return true;
 }
}

S47J::FObj FS47T02PhotoRecord::ToJson() const
{
 const auto O = Photo.ToJson();
 S47J::Set(O, TEXT("v"), 1); S47J::Set(O, TEXT("attemptId"), AttemptId);
 S47J::Set(O, TEXT("unavailable"), bUnavailable); S47J::Set(O, TEXT("imageKey"), ImageKey);
 WriteVector(O, TEXT("sourcePosition"), SourcePosition); WriteVector(O, TEXT("sourceEye"), SourceEye);
 S47J::Set(O, TEXT("sourceYaw"), SourceYaw); S47J::Set(O, TEXT("sourcePitch"), SourcePitch);
 S47J::Set(O, TEXT("sourceFloorY"), SourceFloorY);
 S47J::Set(O, TEXT("cameraRotation"), TArray<double>{CameraRotation.Pitch, CameraRotation.Yaw, CameraRotation.Roll});
 const auto Points = S47J::New();
 for (const auto& Feature : Features) WriteVector(Points, *Feature.Key, Feature.Value);
 S47J::Set(O, TEXT("features"), Points); return O;
}

TOptional<FS47T02PhotoRecord> FS47T02PhotoRecord::FromJson(const S47J::FObj& O)
{
 if (!O || !O->HasTypedField<EJson::Number>(TEXT("v")) || S47J::Num(O, TEXT("v")) != 1 ||
  !O->HasTypedField<EJson::Number>(TEXT("clock")) || !O->HasTypedField<EJson::Number>(TEXT("sourceYaw")) ||
  !O->HasTypedField<EJson::Number>(TEXT("sourcePitch")) || !O->HasTypedField<EJson::Number>(TEXT("sourceFloorY")) ||
  (O->HasField(TEXT("unavailable")) && !O->HasTypedField<EJson::Boolean>(TEXT("unavailable")))) return {};
 const auto P = FS47Photo::FromJson(O); if (!P) return {};
 FS47T02PhotoRecord R; R.Photo = *P;
 R.AttemptId = S47J::Str(O, TEXT("attemptId")); R.ImageKey = S47J::Str(O, TEXT("imageKey"));
 R.bUnavailable = S47J::Bool(O, TEXT("unavailable"));
 if (!ReadVector(O, TEXT("sourcePosition"), R.SourcePosition) || !ReadVector(O, TEXT("sourceEye"), R.SourceEye)) return {};
 FVector Rotation;
 if (!ReadVector(O, TEXT("cameraRotation"), Rotation)) return {};
 R.CameraRotation = FRotator(Rotation.X, Rotation.Y, Rotation.Z);
 R.SourceYaw = S47J::Num(O, TEXT("sourceYaw")); R.SourcePitch = S47J::Num(O, TEXT("sourcePitch"));
 R.SourceFloorY = S47J::Num(O, TEXT("sourceFloorY"));
 if (const auto Points = S47J::Obj(O, TEXT("features")))
  for (const auto& Entry : Points->Values)
  {
   const FString Key(Entry.Key.ToView()); FVector Point;
   if (!ReadVector(Points, *Key, Point)) return {};
   R.Features.Add({Key, Point});
  }
 // Keep the unavailable record and original ID/URL for SaveStore recovery, but never
 // turn its readable stand-in into evidence. The expected dimensions also reject that print.
 R.bImageDecoded = !R.bUnavailable && !R.ImageKey.IsEmpty() &&
  R.ImageKey == S47Saves::PhotoKey(R.Photo.Url) && DecodePrint(R.Photo.Url);
 if (R.bImageDecoded) R.DecodedUrl = R.Photo.Url;
 else R.bUnavailable = true;
 return R;
}

bool FS47T02PhotoRecord::Valid(const FString& ExpectedAttemptId, const FVector& Reference, double Radius,
 const TArray<TPair<FString, FVector>>& RequiredFeatures) const
{
 if (bUnavailable || !bImageDecoded || DecodedUrl != Photo.Url || ImageKey.IsEmpty() ||
  ExpectedAttemptId.IsEmpty() || AttemptId != ExpectedAttemptId ||
  !Photo.Id.StartsWith(TEXT("t02-")) || Photo.Subject != TEXT("Caliche Junction / local follow-up reference") ||
  Photo.Method != TEXT("t02-instant") || !FMath::IsFinite(Photo.Clock) ||
  !Finite(SourcePosition) || !Finite(SourceEye) || !FiniteRotation(CameraRotation) ||
  !FMath::IsFinite(SourceYaw) || !FMath::IsFinite(SourcePitch) || !FMath::IsFinite(SourceFloorY) ||
  !Finite(Reference) || !FMath::IsFinite(Radius) || Radius <= 0 ||
  FVector2D(SourcePosition.X - Reference.X, SourcePosition.Z - Reference.Z).Size() > Radius ||
  FVector2D(SourceEye.X - SourcePosition.X, SourceEye.Z - SourcePosition.Z).Size() > .25 ||
  SourceEye.Y - SourceFloorY < 1.3 || SourceEye.Y - SourceFloorY > 1.95 || FMath::Abs(SourceFloorY - Reference.Y) > 1 ||
  RequiredFeatures.Num() < 3 || Features.Num() != RequiredFeatures.Num() || Photo.Targets.Num() != RequiredFeatures.Num()) return false;
 TSet<FString> Seen;
 for (const auto& Required : RequiredFeatures)
 {
  if (Required.Key.IsEmpty() || Seen.Contains(Required.Key) || !Finite(Required.Value)) return false;
  Seen.Add(Required.Key);
  const auto* World = Features.FindByPredicate([&](const auto& F) { return F.Key == Required.Key; });
  const auto* UV = Photo.Targets.FindByPredicate([&](const auto& T) { return T.Key == Required.Key; });
  if (!World || !UV || !World->Value.Equals(Required.Value, .03) ||
   !FMath::IsFinite(UV->Value.X) || !FMath::IsFinite(UV->Value.Y)) return false;
  const FS47Projection Projected = ProjectAt(*this, World->Value);
  if (!Inside(Projected) || FVector2D(Projected.U - UV->Value.X, Projected.V - UV->Value.Y).Size() > .0025) return false;
 }
 return true;
}

bool FS47T02Photo::InContext(US47Main* M)
{
 return M && M->Mode == US47Main::EMode::Play && M->Substation && M->World &&
  !M->World->bDriving && M->Substation->FollowupCameraContext();
}

bool FS47T02Photo::CheckView(US47Main* M, FString& Reason)
{
 FS47T02PhotoRecord GeometryRecord;
 return Geometry(M, Reason, GeometryRecord);
}

bool FS47T02Photo::Ready(US47Main* M, FString& Reason)
{
 if (!InContext(M)) { Reason = TEXT("THE T02 INSTANT BACK IS FOR THE CALICHE REFERENCE POSITION"); return false; }
 if (!M->Substation->CanCaptureFollowup(Reason)) return false;
 return CheckView(M, Reason);
}

bool FS47T02Photo::TryShutter(US47Main* M)
{
 if (!InContext(M) || !M->FieldCamera || !M->FieldCamera->bRaised) return false;
 auto Refuse = [M](const FString& Why)
 {
  if (M->UI) M->UI->Toast(Why, 3.5f);
  UE_LOG(LogS47T02Photo, Display, TEXT("S47: T02 reserved photo refused: %s"), *Why);
  return true;
 };
 FString Reason;
 if (!M->Story || M->ModalOpen() || !M->Substation->CanCaptureFollowup(Reason))
  return Refuse(Reason.IsEmpty() ? FString(TEXT("COMPLETE THE CURRENT LOCAL CONTROLS BEFORE EXPOSING THE INSPECTION PRINT")) : Reason);
 FS47T02PhotoRecord Record;
 if (!Geometry(M, Reason, Record)) return Refuse(Reason);
 if (Record.AttemptId.IsEmpty()) return Refuse(TEXT("START A CURRENT LOCAL INSPECTION ATTEMPT FIRST"));
 Record.Photo.Id = TEXT("t02-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
 Record.Photo.Subject = TEXT("Caliche Junction / local follow-up reference");
 Record.Photo.Method = TEXT("t02-instant"); Record.Photo.Clock = M->Story->Clock();
 // A transient native flash belongs to this exposure only. No existing lamp or film-only
 // object is borrowed; After/Restore both release it, including an unsuccessful render.
 USpotLightComponent* Flash = NewObject<USpotLightComponent>(M->Web, NAME_None, RF_Transient);
 if (!Flash) return Refuse(TEXT("THE CAMERA FLASH IS NOT AVAILABLE. NO INSPECTION PRINT RETAINED."));
 Flash->SetupAttachment(M->Web->GetRootComponent());
 Flash->SetMobility(EComponentMobility::Movable); Flash->SetIntensityUnits(ELightUnits::Candelas);
 Flash->SetIntensity(0); Flash->SetLightColor(S47Three::Hex(0xfff6ec));
 Flash->SetAttenuationRadius(5000); Flash->SetOuterConeAngle(55); Flash->SetInnerConeAngle(42);
 Flash->SetSourceRadius(1.5f); Flash->SetCastShadows(true); Flash->SetVolumetricScatteringIntensity(0);
 Flash->SetWorldLocationAndRotation(M->FieldCamera->PhotoLoc, Record.CameraRotation); Flash->RegisterComponent();
 if (!Flash->IsRegistered()) { Flash->DestroyComponent(); return Refuse(TEXT("THE CAMERA FLASH COULD NOT START. NO INSPECTION PRINT RETAINED.")); }
 const TWeakObjectPtr<USpotLightComponent> WeakFlash(Flash);
 const TSharedRef<bool> Released = MakeShared<bool>(false);
 const auto ReleaseFlash = [WeakFlash, Released]
 {
  if (*Released) return;
  *Released = true;
  if (USpotLightComponent* Light = WeakFlash.Get()) { Light->SetIntensity(0); Light->DestroyComponent(); }
 };
 FS47ExposeOptions Options;
 Options.Before = [WeakFlash] { if (USpotLightComponent* Light = WeakFlash.Get()) Light->SetIntensity(1800); };
 Options.After = ReleaseFlash; Options.Restore = ReleaseFlash;
 Options.Caption = TEXT("CALICHE // LOCAL FOLLOW-UP"); Options.Time = M->Story->ClockText(true);
 Options.Seed = FMath::FloorToDouble(Record.Photo.Clock * 1000);
 TArray<FColor> Print;
 const bool Rendered = M->FieldCamera->Expose(Options, Print);
 ReleaseFlash();
 if (!Rendered || Print.Num() != US47FieldCamera::PRINT_W * US47FieldCamera::PRINT_H)
  return Refuse(TEXT("THE INSPECTION EXPOSURE COULD NOT BE RENDERED. RETAKE FROM THIS POSITION."));
 Record.Photo.Url = S47Image::ToDataUrl(Print, US47FieldCamera::PRINT_W, US47FieldCamera::PRINT_H, true, 86);
 Record.ImageKey = S47Saves::PhotoKey(Record.Photo.Url);
 Record.bImageDecoded = DecodePrint(Record.Photo.Url);
 if (Record.bImageDecoded) Record.DecodedUrl = Record.Photo.Url;
 if (!Record.Valid(M->Substation->FollowupAttemptId(), M->Substation->FollowupReferencePosition(),
  M->Substation->FollowupReferenceRadius(), M->Substation->FollowupFeatures()))
  return Refuse(TEXT("THE INSPECTION PRINT COULD NOT BE RETAINED AS CURRENT EVIDENCE. RETAKE IT."));
 if (!M->Substation->OnFollowupCapture(Record))
  return Refuse(TEXT("THE LOCAL CHECK CHANGED BEFORE THE PRINT WAS RETAINED. REPEAT THE CURRENT COMPARISON."));
 if (M->Audio) M->Audio->Shutter();
 if (M->UI) { M->UI->ShutterFlash(); M->UI->Toast(TEXT("Inspection instant print retained with the current local check."), 4); }
 M->RequestAutosave();
 UE_LOG(LogS47T02Photo, Display, TEXT("S47: T02 reserved photo exposed: %s, attempt %s, clock %.3f, source %.3f %.3f %.3f, %d actual feature UVs, %d JPEG bytes"),
  *Record.Photo.Id, *Record.AttemptId, Record.Photo.Clock, Record.SourcePosition.X, Record.SourceFloorY,
  Record.SourcePosition.Z, Record.Photo.Targets.Num(), Record.Photo.Url.Len());
 return true;
}

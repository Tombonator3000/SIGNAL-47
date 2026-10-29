#include "S47SC10Contract.h"
#include "S47Chapter4.h"
#include "S47Chapter1.h"
#include "S47Chapter2.h"
#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Motel.h"
#include "S47NpcDialogue.h"
#include "S47Room6.h"
#include "S47Spine.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "UnrealClient.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
 void SC10Text(const TSharedRef<SWidget>& Widget, TArray<FString>& Text)
 {
  if (!Widget->GetVisibility().IsVisible()) return;
  if (Widget->GetTypeAsString() == TEXT("STextBlock")) Text.Add(StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString());
  FChildren* Children = Widget->GetChildren();
  for (int32 Index = 0; Children && Index < Children->Num(); ++Index) SC10Text(Children->GetChildAt(Index), Text);
 }
 bool SC10Png(const FString& Path, S47J::FObj& Receipt)
 {
  TArray<uint8> Bytes; if (!FFileHelper::LoadFileToArray(Bytes, *Path)) return false;
  const uint8 Signature[] = {137,80,78,71,13,10,26,10};
  if (Bytes.Num() < 33 || FMemory::Memcmp(Bytes.GetData(), Signature, 8)) return false;
  const auto U32 = [&](int64 Index) { return (uint32(Bytes[Index]) << 24) | (uint32(Bytes[Index+1]) << 16) | (uint32(Bytes[Index+2]) << 8) | Bytes[Index+3]; };
  int64 Offset = 8, DataBytes = 0; int32 Chunks = 0; uint32 Width = 0, Height = 0; bool End = false;
  while (Offset + 12 <= Bytes.Num())
  {
   const uint32 Size = U32(Offset); if (int64(Size) > Bytes.Num() - Offset - 12) return false;
   const uint8* Type = Bytes.GetData() + Offset + 4;
   if (FCrc::MemCrc32(Type, Size + 4) != U32(Offset + 8 + Size)) return false;
   if (!Chunks) { if (FMemory::Memcmp(Type,"IHDR",4) || Size != 13) return false; Width = U32(Offset+8); Height = U32(Offset+12); }
   if (!FMemory::Memcmp(Type,"IDAT",4)) DataBytes += Size;
   ++Chunks; Offset += int64(Size) + 12;
   if (!FMemory::Memcmp(Type,"IEND",4)) { End = Size == 0; break; }
  }
  if (!End || Offset != Bytes.Num() || !DataBytes || Width < 320 || Height < 200) return false;
  FSHAHash Hash; FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Hash.Hash); Receipt = S47J::New();
  S47J::Set(Receipt,TEXT("path"),Path); S47J::Set(Receipt,TEXT("bytes"),Bytes.Num()); S47J::Set(Receipt,TEXT("sha1"),Hash.ToString());
  S47J::Set(Receipt,TEXT("width"),double(Width)); S47J::Set(Receipt,TEXT("height"),double(Height));
  S47J::Set(Receipt,TEXT("crc_iend_eof"),true); return true;
 }
}

// The prior chapters and the exterior start are explicit fixtures. Inside Room 6, movement is
// Walker input/Resolve and every world use has the actual camera ray and ordinary HUD OnUse.
// Conversation/board steps call the same offered production callbacks as the Slate panel.
class FS47SC10Contract
{
public:
 explicit FS47SC10Contract(US47Main* Main) : M(Main) {}
 void Step(double Dt);
private:
 US47Main* M = nullptr;
 FString Root, PendingRestore;
 int32 Stage = 0, Checks = 0, Stores = 0, Uses = 0, WalkSamples = 0, Waypoint = 0;
 double Began = 0, Hold = 0, ProgressAt = 0, Walked = 0;
 bool Started = false, Done = false, bNormalContinue = false;
 FVector LastWalk = FVector::ZeroVector, Aim = FVector::ZeroVector;
 TArray<FVector2D> Route;
 TMap<FString,int32> OriginalNodes;
 S47J::FObj Expected;
 TArray<TSharedPtr<FJsonValue>> Executed, Cohorts, Rays, Photos;
 TArray<FString> PhotoFiles;

 US47Chapter4* Chapter() const { return M && M->Story ? Cast<US47Chapter4>(M->Story->Chapter(TEXT("ch4"))) : nullptr; }
 void Result(bool Good, const FString& Why);
 bool Check(bool Good, const FString& Why);
 void Next(int32 Value, double Wait = .3) { Stage = Value; Hold = Wait; }
 void Refresh() { if (US47Chapter4* Part = Chapter(); Part && Part->Panel && Part->Panel->Refresh) Part->Panel->Refresh(); }
 bool ModalHas(const FString& Text) const;
 bool RoomFacts(const FString& Context);
 bool Restore(const FString& Name);
 bool Restored();
 bool Choice(const FString& Id);
 bool Board(const FString& A, const FString& B, const FString& Question);
 void Path(TArray<FVector2D> Points);
 bool Walk();
 void Face(const FString& Id);
 bool Use(const FString& Id);
 void Shot(const FString& Name);
};

void FS47SC10Contract::Result(bool Good, const FString& Why)
{
 if (Done) return; Done = true;
 if (M && M->Player) { M->Player->TestMove.Reset(); M->Player->bTestRun = false; }
 auto Report = S47J::New(); S47J::Set(Report,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));
 S47J::Set(Report,TEXT("scenario"),TEXT("SC10")); S47J::Set(Report,TEXT("reason"),Why); S47J::Set(Report,TEXT("stage"),Stage);
 S47J::Set(Report,TEXT("checks"),Checks); S47J::Set(Report,TEXT("stores"),Stores); S47J::Set(Report,TEXT("actual_ray_uses"),Uses);
 S47J::Set(Report,TEXT("walked_metres"),Walked); S47J::Set(Report,TEXT("walk_samples"),WalkSamples);
 S47J::Set(Report,TEXT("normal_title_continue"),bNormalContinue); S47J::Set(Report,TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Began);
 S47J::Set(Report,TEXT("method"),TEXT("Isolated chapter4/prior-evidence fixture and one exterior Room6 start; ordinary door entry; continuous native Walker inside; actual camera-ray/HUD uses; offered talk and rule-book callbacks; fresh disk readers/full Flow restores and ordinary title Continue."));
 S47J::Set(Report,TEXT("limits"),TEXT("SC01-SC09 and the highway drive are prerequisites supplied by the developer fixture, not replayed. API-driven input, not human input. No Nora recording played. PNG integrity is not visual or performance acceptance."));
 Report->SetArrayField(TEXT("executed_checks"),Executed); Report->SetArrayField(TEXT("restores"),Cohorts);
 Report->SetArrayField(TEXT("rays"),Rays); Report->SetArrayField(TEXT("photos"),Photos);
 const bool Written = !Root.IsEmpty() && FFileHelper::SaveStringToFile(S47J::Write(Report,true),*(Root/TEXT("result.json")));
 if (Good && Written) UE_LOG(LogTemp,Display,TEXT("S47: spine contract sc10 PASS COMPLETE: %d checks, %d stores, %d actual uses, %.3fm, normalContinue=PASS evidence=%s"),Checks,Stores,Uses,Walked,*Root);
 else UE_LOG(LogTemp,Error,TEXT("S47: spine contract sc10 FAIL stage=%d checks=%d: %s (result written %d)"),Stage,Checks,*Why,Written);
 FPlatformMisc::RequestExitWithStatus(false,Good && Written ? 0 : 1);
}

bool FS47SC10Contract::Check(bool Good, const FString& Why)
{
 ++Checks; auto Row = S47J::New(); S47J::Set(Row,TEXT("check"),Checks); S47J::Set(Row,TEXT("stage"),Stage);
 S47J::Set(Row,TEXT("pass"),Good); S47J::Set(Row,TEXT("detail"),Why); Executed.Add(MakeShared<FJsonValueObject>(Row));
 if (!Good) { Result(false,Why); return false; }
 UE_LOG(LogTemp,Display,TEXT("S47: SC10 PASS: %s"),*Why); return true;
}

bool FS47SC10Contract::ModalHas(const FString& Text) const
{
 const auto Widget = M->Hud ? M->Hud->ModalWidget() : nullptr; TArray<FString> Found;
 if (Widget) SC10Text(Widget.ToSharedRef(),Found);
 return Found.ContainsByPredicate([&](const FString& Value) { return Value.Contains(Text); });
}

bool FS47SC10Contract::RoomFacts(const FString& Context)
{
 US47Chapter4* Part = Chapter(); if (!Check(Part && Part->Room && Part->Court,Context+TEXT(" room owners exist"))) return false;
 for (const TCHAR* Name : {TEXT("nora"),TEXT("table"),TEXT("phone"),TEXT("door")})
 {
  const FS47Obj Object = Part->Room->Proxy(Name); const FString Key = FString(TEXT("proxy:"))+Name;
  if (!Check(Object && (!OriginalNodes.Contains(Key) || OriginalNodes[Key] == Object.I),Context+TEXT(" original ")+Key)) return false;
  OriginalNodes.Add(Key,Object.I);
 }
 const struct { const TCHAR* Name; bool Shown; } Papers[] = {
  {TEXT("correction"),Part->S.Topics.bRecord},{TEXT("letter"),Part->S.Topics.bTomas},{TEXT("fieldCard"),Part->S.Topics.bCable}};
 for (const auto& Paper : Papers)
 {
  const FS47Obj Object = Part->Room->Obj(Paper.Name); const FString Key = FString(TEXT("paper:"))+Paper.Name;
  if (!Check(Object && Object.Visible() == Paper.Shown && (!OriginalNodes.Contains(Key) || OriginalNodes[Key] == Object.I),Context+TEXT(" original paper visibility/identity ")+Paper.Name)) return false;
  OriginalNodes.Add(Key,Object.I);
 }
 return Check(Part->TextOnlyNora() && (!M->UI->NpcDialogue || !M->UI->NpcDialogue->Playing()),Context+TEXT(" Nora is text; no recording playback claimed"));
}

bool FS47SC10Contract::Restore(const FString& Name)
{
 M->UI->Close(true); Expected = S47J::Clone(M->Flow->GameState());
 FS47SaveStore Writer(Root/TEXT("cohorts")/Name); Writer.Init(); FS47SaveCard Card;
 Card.Chapter = TEXT("SC10 Room 6"); Card.Place = M->Flow->PlaceName(); Card.Clock = M->Story->ClockText(true);
 FString Error; const auto Meta = Writer.Save(1,TEXT("manual"),0,Expected,Card,Error);
 if (!Check(Meta.IsSet(),TEXT("isolated disk write ")+Name+TEXT(": ")+Error)) return false;
 FS47SaveStore Reader(Root/TEXT("cohorts")/Name); Reader.Init(); const auto Disk = Reader.Load(Meta->Id);
 if (!Check(Disk && S47J::Write(Disk) == S47J::Write(Expected),TEXT("fresh reader retains full disk state ")+Name)) return false;
 if (!Check(FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/(Name+TEXT(".disk.json")))),TEXT("persist raw disk cohort ")+Name)) return false;
 PendingRestore = Name; ++Stores;
 US47Flow::FStart Start; Start.State = Disk; Start.CaseId = 0; Start.bDev = true; Start.Playtime = M->Playtime;
 M->Flow->StartGame(Start); return true;
}

bool FS47SC10Contract::Restored()
{
 const auto SavedCase = S47J::Obj(Expected,TEXT("case")); const auto Pose = S47J::Obj(Expected,TEXT("pose"));
 US47Chapter4* Part = Chapter(); const auto Now = M->Flow->GameState();
 if (!Check(Part && M->Story->bEveningTimeline && M->World->Area == S47J::Str(Expected,TEXT("area"))
  && S47J::Write(Part->SaveState()) == S47J::Write(S47J::Obj(SavedCase,TEXT("ch4"))),TEXT("full Flow keeps SC10 state/area/timeline: ")+PendingRestore)) return false;
 if (!Check(M->Story->Clock() >= S47J::Num(SavedCase,TEXT("clock")) && M->Story->Clock()-S47J::Num(SavedCase,TEXT("clock")) < 3
  && FMath::Abs(M->Player->Pos.X-S47J::Num(Pose,TEXT("x"))) < .03 && FMath::Abs(M->Player->Pos.Z-S47J::Num(Pose,TEXT("z"))) < .03
  && FMath::Abs(M->Player->Yaw-S47J::Num(Pose,TEXT("yaw"))) < .001
  && S47J::Write(S47J::Obj(Now,TEXT("doors"))) == S47J::Write(S47J::Obj(Expected,TEXT("doors"))),TEXT("disk restore retains clock/pose/doors: ")+PendingRestore)) return false;
 const auto CurrentCase = S47J::Obj(Now,TEXT("case"));
 for (const TCHAR* Key : {TEXT("s"),TEXT("ch2"),TEXT("ch3")})
  if (!Check(S47J::Write(S47J::Obj(CurrentCase,Key)) == S47J::Write(S47J::Obj(SavedCase,Key)),TEXT("prior chapter evidence unchanged: ")+FString(Key)+TEXT(" / ")+PendingRestore)) return false;
 if (!RoomFacts(TEXT("restored ")+PendingRestore)) return false;
 auto Receipt = S47J::New(); S47J::Set(Receipt,TEXT("name"),PendingRestore); S47J::Set(Receipt,TEXT("pass"),true);
 S47J::Set(Receipt,TEXT("restored_ch4"),Part->SaveState()); S47J::Set(Receipt,TEXT("clock"),M->Story->Clock());
 Cohorts.Add(MakeShared<FJsonValueObject>(Receipt)); PendingRestore.Reset(); return true;
}

bool FS47SC10Contract::Choice(const FString& Id)
{
 US47Chapter4* Part = Chapter(); const auto View = Part->View();
 if (!Check(M->UI->Modal() && View.Choices.ContainsByPredicate([&](const S47Panels::FTalkChoice& Value) { return Value.Id == Id && !Value.bDisabled; }),TEXT("ordinary offered dialogue choice: ")+Id)) return false;
 Part->Choose(Id); Refresh(); return true;
}

bool FS47SC10Contract::Board(const FString& A, const FString& B, const FString& Question)
{
 US47Chapter4* Part = Chapter(); const auto Connected = Part->Book->Connect(A,B,false);
 if (!Check(Connected && Connected->bOk,TEXT("production board evidence thread: ")+A+TEXT(" / ")+B)) return false;
 if (!Question.IsEmpty())
 {
  const auto Recorded = Part->Book->Record(Question);
  if (!Check(Recorded && Recorded->bOk,TEXT("production board records ")+Question)) return false;
 }
 Refresh(); return true;
}

void FS47SC10Contract::Path(TArray<FVector2D> Points)
{
 Route = MoveTemp(Points); Waypoint = 0; LastWalk = M->Player->Pos; ProgressAt = FPlatformTime::Seconds();
}
bool FS47SC10Contract::Walk()
{
 AS47Walker* Player = M->Player; const double Distance = FVector2D(Player->Pos.X-LastWalk.X,Player->Pos.Z-LastWalk.Z).Size();
 if (Distance >= .65) { Result(false,TEXT("native Walker moved farther than one continuous step")); return false; }
 Walked += Distance; LastWalk = Player->Pos; ++WalkSamples;
 if (FPlatformTime::Seconds()-ProgressAt > 18) { Result(false,TEXT("physical room route stalled; no placement recovery allowed")); return false; }
 const FVector2D Delta = Route[Waypoint]-FVector2D(Player->Pos.X,Player->Pos.Z);
 if (Delta.Size() < .09)
 {
  Player->TestMove.Reset(); ++Waypoint; ProgressAt = FPlatformTime::Seconds();
  if (Waypoint == Route.Num()) return true;
  return false;
 }
 Player->Yaw = FMath::Atan2(-Delta.X,-Delta.Y);
 Player->TestMove = FVector2D(0,-FMath::Clamp(Delta.Size()/.5,0.,1.)); Player->bTestRun = false; return false;
}

void FS47SC10Contract::Face(const FString& Id)
{
 const auto* Item = M->Inter->Get(Id); if (!Item) { Result(false,TEXT("missing original interaction: ")+Id); return; }
 const FBox Bounds = Item->Object.WorldBox(); if (!Bounds.IsValid) { Result(false,TEXT("missing original interaction bounds: ")+Id); return; }
 FVector TargetThree = Bounds.GetCenter();
 if (Id == TEXT("r6:nora")) TargetThree.Y += Bounds.GetExtent().Y*.55;
 Aim = S47Three::ToUE(TargetThree); // FS47Obj::WorldBox is three metres, not an Unreal centimetre box.
 const FVector From = S47Three::ToThree(M->Player->EyeUE()), Target = S47Three::ToThree(Aim);
 M->Player->Yaw = FMath::Atan2(-(Target.X-From.X),-(Target.Z-From.Z));
 M->Player->Pitch = FMath::Atan2(Target.Y-From.Y,FVector2D(Target.X-From.X,Target.Z-From.Z).Size());
}

bool FS47SC10Contract::Use(const FString& Id)
{
 const FVector Eye = M->Player->EyeUE(), Forward = M->Player->ForwardUE();
 const FString Picked = M->Inter->Pick(Eye,Forward); const FString Focused = M->Inter->Update(Eye,Forward);
 auto Row = S47J::New(); S47J::Set(Row,TEXT("expected"),Id); S47J::Set(Row,TEXT("picked"),Picked); S47J::Set(Row,TEXT("focused"),Focused);
 S47J::Set(Row,TEXT("eye_ue_cm"),Eye.ToString()); S47J::Set(Row,TEXT("forward_ue"),Forward.ToString());
 S47J::Set(Row,TEXT("target_distance_m"),FVector::Dist(Eye,Aim)/100.); Rays.Add(MakeShared<FJsonValueObject>(Row));
 if (!Check(Picked == Id && Focused == Id && M->Player->OnUse && FVector::DotProduct(Forward,(Aim-Eye).GetSafeNormal()) > .995,TEXT("actual camera ray/focus/HUD OnUse: ")+Id)) return false;
 M->Player->OnUse(Picked); ++Uses; return true;
}

void FS47SC10Contract::Shot(const FString& Name)
{
 const FString File = Root/(Name+TEXT(".png")); PhotoFiles.Add(File); FScreenshotRequest::RequestScreenshot(File,true,false);
}

void FS47SC10Contract::Step(double Dt)
{
 if (Done || !M || !M->Story || !M->World || !M->Player || !M->Flow || !M->UI || !M->Inter || !M->Web || !M->Spine) return;
 if (!Started)
 {
  if (S47Startup::Blocking() || M->Flow->bStarting || M->World->bBusy || M->Mode != US47Main::EMode::Play) return;
  Root = FPaths::ProjectSavedDir()/TEXT("SC10Contract")/FGuid::NewGuid().ToString(EGuidFormats::Digits); IFileManager::Get().MakeDirectory(*Root,true);
  Started = true; Began = FPlatformTime::Seconds(); FString UserDir, Jump;
  FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir); FParse::Value(FCommandLine::Get(),TEXT("S47Jump="),Jump);
  UserDir = FPaths::ConvertRelativePathToFull(UserDir); FPaths::CollapseRelativeDirectories(UserDir);
  if (!Check(M->Flow->bDevRun && M->Flow->CaseId == 0 && Jump == TEXT("chapter4") && (UserDir.StartsWith(TEXT("/tmp/")) || UserDir.StartsWith(TEXT("/var/tmp/")))
   && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),UserDir),TEXT("explicit isolated case0 chapter4 fixture; no player save"))) return;
  if (!Check(M->Flow->Saves.List().IsEmpty(),TEXT("initial isolated profile has no saved cases"))) return;
  US47Chapter4* Part = Chapter();
  if (!Check(Part && Part->bActive && M->Spine->Enabled() && M->Story->bEveningTimeline && M->Story->Clock() >= S47Timeline::Chapter4(true)
   && M->Story->Clock() < 5*3600 && S47J::Num(Part->Ch3(),TEXT("dev")) >= 3 && S47J::Bool(Part->Ch3(),TEXT("called")),TEXT("04:50 SC10 fixture retains both preceding secured field prints and completed Nora call"))) return;
  for (const TCHAR* Flag : {TEXT("S47Ch4Drive"),TEXT("S47MotelTv"),TEXT("S47MotelV2Route"),TEXT("S47HorrorContract")})
  { FString Value; if (FParse::Param(FCommandLine::Get(),Flag) || FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),Value)) { Result(false,TEXT("independent driver also enabled")); return; } }
  const FS47Pose Outside = Part->Court->Anchor(TEXT("room6Outside"));
  if (!Check(Outside.bValid && M->Player->Walkable(Outside.X,Outside.Z),TEXT("one legal exterior fixture at existing room6Outside anchor"))) return;
  M->Player->Place(Outside.X,Outside.Z,Outside.Yaw); Next(1,.5); return;
 }
 if (FPlatformTime::Seconds()-Began > 180) { Result(false,TEXT("180s SC10 timeout")); return; }
 if (M->Flow->bStarting || S47Startup::Blocking() || M->World->bBusy) return;
 if (Hold > 0) { Hold -= Dt; return; }
 if (!PendingRestore.IsEmpty() && !Restored()) return;
 US47Chapter4* Part = Chapter(); if (!Part) { Result(false,TEXT("SC10 owner disappeared")); return; }
 const auto Talk = Part->Room->Anchor(TEXT("talk")), Arrive = Part->Room->Anchor(TEXT("arrive")), Exit = Part->Room->Anchor(TEXT("exit"));
 const double Ox = Arrive.X+1.4, Oz = Arrive.Z-4.3;
 switch (Stage)
 {
 case 1: Face(TEXT("room6Door")); Next(2,.4); return;
 case 2: if (Use(TEXT("room6Door"))) Next(3,.9); return;
 case 3:
  if (!Check(M->World->Area == TEXT("room6") && Part->S.bArrived && Part->S.Stage == TEXT("room"),TEXT("ordinary exterior door enters safe Room 6")) || !RoomFacts(TEXT("entry"))) return;
  Path({FVector2D(Arrive.X,Oz+1.28),FVector2D(Talk.X,Talk.Z)}); Next(4,0); return;
 case 4: if (Walk()) { Face(TEXT("r6:nora")); Next(5,.4); } return;
 case 5: if (Use(TEXT("r6:nora"))) Next(6,1.2); return;
 case 6:
  if (!Check(ModalHas(TEXT("So you came down the road.")) && Part->S.bGreeted && Part->Mode == TEXT("table"),TEXT("actual Nora modal begins with the two prints and original"))) return;
  Shot(TEXT("room6-greeting")); Next(31,.6); return;
 case 31:
  Part->ToggleChip(TEXT("e07")); Part->ShowEvidence(); Refresh();
  if (!Check(!Part->S.bP10 && Part->Reply && Part->Reply->Text.Contains(TEXT("Both prints")),TEXT("one original cannot answer P10"))) return;
  if (Restore(TEXT("greeted-table"))) Next(7,.4); return;
 case 7: Face(TEXT("r6:nora")); Next(8,.4); return;
 case 8: if (Use(TEXT("r6:nora"))) Next(9,.5); return;
 case 9:
  if (!Check(Part->View().Table.IsSet() && Part->Mode == TEXT("table") && Part->S.Placed.Contains(TEXT("e07")),TEXT("Continue before P10 restores the evidence table and selected original"))) return;
  Part->Choose(TEXT("end")); if (!Check(!Part->S.bTold,TEXT("unoffered ending cannot skip the evidence"))) return;
  for (const TCHAR* Chip : {TEXT("frame01"),TEXT("frame02")}) Part->ToggleChip(Chip);
  Part->ShowEvidence(); Refresh();
  if (!Check(Part->S.bShown && !Part->S.bP10 && Part->Mode == TEXT("control"),TEXT("both B-12 prints and E07 enable the real control question"))) return;
  if (Restore(TEXT("control-question"))) Next(10,.4); return;
 case 10: Face(TEXT("r6:nora")); Next(11,.4); return;
 case 11: if (Use(TEXT("r6:nora"))) Next(12,.5); return;
 case 12:
  if (!Check(Part->Mode == TEXT("control") && !Part->View().Question.IsEmpty(),TEXT("Continue restores the unanswered control question"))) return;
  if (!Choice(TEXT("receiver")) || !Check(!Part->S.bP10 && Part->Reply.IsSet(),TEXT("wrong control remains unanswered")) || !Choice(TEXT("lamp"))) return;
  if (!Check(Part->S.bP10,TEXT("passive fixture accepts only the shielded-lamp control")) || !Choice(TEXT("tomas")) || !Choice(TEXT("record")) || !Choice(TEXT("critical"))) return;
  if (Restore(TEXT("two-topics"))) Next(13,.4); return;
 case 13: Face(TEXT("r6:nora")); Next(14,.4); return;
 case 14: if (Use(TEXT("r6:nora"))) Next(15,.5); return;
 case 15:
  if (!Check(Part->S.Topics.bTomas && Part->S.Topics.bRecord && !Part->S.Topics.bCable && Part->S.Tone == TEXT("critical"),TEXT("Tomás letter, signed correction and chosen tone survive disk restore")) || !Choice(TEXT("cable")) || !RoomFacts(TEXT("three topics"))) return;
  if (!Board(TEXT("e12.why"),TEXT("P04"),{}) || !Board(TEXT("e12.cut"),TEXT("P08"),TEXT("P11"))) return;
  for (const TCHAR* Paper : {TEXT("correction"),TEXT("letter"),TEXT("card")})
  {
   Part->ReadPaper(Paper); if (!Check(M->UI->Modal(),TEXT("ordinary given-paper document opens: ")+FString(Paper))) return;
   M->UI->Close(true);
  }
  if (!Check(Part->S.Read.bCard && Part->S.Read.bLetter && Part->S.Read.bCorrection,TEXT("all three original Nora documents read"))) return;
  Path({FVector2D(Ox-.8,Oz+.4),FVector2D(Ox-.8,Oz-1.5)}); Next(16,0); return;
 case 16: if (Walk()) { Face(TEXT("r6:phone")); Next(17,.4); } return;
 case 17: if (Use(TEXT("r6:phone"))) Next(18,4.8); return;
 case 18:
  if (!Check(Part->S.bLine && ModalHas(TEXT("Tomás. Where did you leave the spare key?")) && ModalHas(TEXT("Reference west. No. East. Hold the last reading.")),TEXT("actual nightstand phone opens new question and unchanged 1947 fragment"))) return;
  if (!Board(TEXT("r6.line"),TEXT("e13.key"),{}) || !Board(TEXT("r6.line"),TEXT("e10.carrier"),TEXT("P12"))) return;
  if (Restore(TEXT("findings-recorded"))) Next(19,.4); return;
 case 19:
  if (!Check(Part->S.bP10 && Part->S.bP11 && Part->S.bP12 && !Part->S.bTold,TEXT("all findings survive while Nora's ending remains unspoken"))) return;
  Path({FVector2D(Ox-.8,Oz+.4),FVector2D(Talk.X,Talk.Z)}); Next(20,0); return;
 case 20: if (Walk()) { Face(TEXT("r6:nora")); Next(21,.4); } return;
 case 21: if (Use(TEXT("r6:nora"))) Next(22,.5); return;
 case 22:
  if (!Choice(TEXT("end"))) return; Next(23,2.7); return;
 case 23:
  if (!Check(Part->S.bTold && Part->S.Stage == TEXT("leave") && Part->Lines.Last().Text == TEXT("Don't finish his observation. Get the originals to the courier, and don't stop at C.")
   && ModalHas(TEXT("Don't finish his observation. Get the originals to the courier, and don't stop at C.")),TEXT("Nora actually ends with the approved courier warning"))) return;
  Shot(TEXT("nora-courier-warning")); Next(32,.6); return;
 case 32:
  M->UI->Close(true);
  M->Flow->Saves = FS47SaveStore(Root/TEXT("normal-save")); M->Flow->Saves.Init(); M->Flow->CaseId = 1; M->Flow->bDevRun = false; M->Flow->Saves.SetActive(1);
  Next(24,.4); return;
 case 24:
 {
  if (!M->Flow->SaveBlock().IsEmpty()) { if (FPlatformTime::Seconds()-Began > 160) Result(false,TEXT("ordinary save gate: ")+M->Flow->SaveBlock()); return; }
  FString Error; const auto Meta = M->Flow->WriteSave(TEXT("manual"),0,Error);
  FS47SaveStore Reader(M->Flow->Saves.Dir); Reader.Init(); const auto Continued = Reader.ContinueSave();
  const auto Manual = Continued ? Reader.Load(Continued->Id) : nullptr;
  if (!Check(Meta && Continued && Manual && Meta->Id == Continued->Id,TEXT("ordinary manual save and independent reader: ")+Error)) return;
  if (!Check(FFileHelper::SaveStringToFile(S47J::Write(Manual,true),*(Root/TEXT("manual-before-title.disk.json"))),TEXT("write actual manual record before title"))) return;
  ++Stores; M->Flow->ToTitle();
  // ToTitle may make its own ordinary autosave. Pin what Continue actually selects afterwards.
  FS47SaveStore TitleReader(M->Flow->Saves.Dir); TitleReader.Init(); const auto Selected = TitleReader.ContinueSave();
  Expected = Selected ? TitleReader.Load(Selected->Id) : nullptr;
  const auto FlowSelected = M->Flow->Saves.ContinueSave();
  if (!Check(M->Mode == US47Main::EMode::Title && Expected && Selected && FlowSelected && Selected->Id == FlowSelected->Id
   && FFileHelper::SaveStringToFile(S47J::Write(Expected,true),*(Root/TEXT("normal-continue.disk.json"))),TEXT("fresh reader pins the actual post-title Continue selection"))) return;
  if (!Check(M->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary title Continue opens the actual disk save"))) return;
  PendingRestore = TEXT("normal-title-continue"); bNormalContinue = true; Next(25,.6); return;
 }
 case 25:
  if (!Check(M->Flow->CaseId == 1 && !M->Flow->bDevRun && Part->S.Stage == TEXT("leave"),TEXT("normal Continue retains the real case and uncompleted exit"))) return;
  Shot(TEXT("room6-after-continue"));
  Path({FVector2D(Arrive.X,Oz+1.28),FVector2D(Exit.X,Exit.Z)}); Next(26,0); return;
 case 26: if (Walk()) { Face(TEXT("r6:door")); Next(27,.4); } return;
 case 27: if (Use(TEXT("r6:door"))) Next(28,.65); return;
 case 28:
  if (!Check(M->World->Area == TEXT("saro") && Part->S.Stage == TEXT("complete") && M->Story->Clock() >= 5*3600,TEXT("normal exit completes SC10 at05:00 without rewinding"))) return;
  // Capture the real pending continuation window, before its 1.2s timer can fire.
  if (Restore(TEXT("complete-before-transition"))) Next(29,.2); return;
 case 29: Next(30,1.5); return;
 case 30:
  if (!Check(M->Spine->Gate(FS47Spine::SC11) == 1 && !M->Story->Begun.Contains(TEXT("ch5")),TEXT("restored completion naturally hands records to SC11; chapter5 remains gated"))) return;
  for (const FString& File : PhotoFiles)
  {
   S47J::FObj Receipt; if (!Check(SC10Png(File,Receipt),TEXT("actual complete PNG/CRC/IEND/SHA: ")+File)) return;
   Photos.Add(MakeShared<FJsonValueObject>(Receipt));
  }
  if (!Check(Stores == 6 && Uses == 8 && Photos.Num() == 3 && Walked > 10 && WalkSamples > 20,TEXT("all six disk cohorts, eight ray uses, three pictures and continuous room route completed"))) return;
  Result(true,TEXT("SC10 full Room6 evidence/conversation/physical entry-exit and disk continuation")); return;
 default: Result(false,TEXT("unknown SC10 test phase")); return;
 }
}

void S47StepSC10Contract(US47Main* Main, double Dt)
{
 static TWeakObjectPtr<US47Main> Owner; static TSharedPtr<FS47SC10Contract> Test;
 if (Owner.Get() != Main) { Owner = Main; Test = MakeShared<FS47SC10Contract>(Main); }
 if (Test) Test->Step(Dt);
}

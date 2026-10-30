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
#include "S47ThreatNavigation.h"
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
 S47J::FObj SC10File(const FString& Path)
 {
  TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*Path))return nullptr;FSHAHash Hash;FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash);
  auto O=S47J::New();S47J::Set(O,TEXT("path"),Path);S47J::Set(O,TEXT("bytes"),Bytes.Num());S47J::Set(O,TEXT("sha1"),Hash.ToString());return O;
 }
 bool SC10Same(const S47J::FObj& A,const S47J::FObj& B)
 {return A.IsValid()==B.IsValid()&&(!A||S47J::Write(A)==S47J::Write(B));}
 bool SC10Copy(const FString& From,const FString& To)
 {TArray<uint8> Bytes;return FFileHelper::LoadFileToArray(Bytes,*From)&&FFileHelper::SaveArrayToFile(Bytes,*To);}
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
 FString Root, PendingRestore, ChainId;
 int32 Stage = 0, Checks = 0, Stores = 0, Uses = 0, WalkSamples = 0, Waypoint = 0;
 double Began = 0, Hold = 0, ProgressAt = 0, Walked = 0;
 bool Started = false, Done = false, bNormalContinue = false, bActual = false, bDrove = false;
 double DriveOdo = 0, DriveMetres = 0;
 FVector LastWalk = FVector::ZeroVector, Aim = FVector::ZeroVector;
 TArray<FVector2D> Route;
 TMap<FString,int32> OriginalNodes;
 S47J::FObj Expected;
 S47J::FObj PriorState, Prerequisite;
 TMap<FString,FString> PriorDocs, ExpectedDocs;
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
 TMap<FString,FString> Documents() const;
 bool Papers(const TMap<FString,FString>& Want,const FString& Context);
 bool Segment(const FVector2D& A,const FVector2D& B) const;
 bool Go(const FString& Id);
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
 S47J::Set(Report,TEXT("mode"),bActual?TEXT("actual-checkpoint"):TEXT("named-chapter4-fixture"));S47J::Set(Report,TEXT("chain_id"),ChainId);S47J::Set(Report,TEXT("prerequisite"),Prerequisite);
 S47J::Set(Report,TEXT("source_method"),bActual?TEXT("Actual SC09 PASS/checkpoint chain; full source-preserving case1 disk/title/Continue, ordinary API World drive and native Walker/ray uses. No chapter/exterior placement fixture."):TEXT("Named chapter4 and exterior prerequisite fixture; physical Room6 actions and disk cohorts."));
 S47J::Set(Report,TEXT("checkpoint_file"),Root.IsEmpty()?nullptr:SC10File(Root/TEXT("sc10-complete.disk.json")));S47J::Set(Report,TEXT("final_state"),M&&M->Flow?M->Flow->GameState():nullptr);
 S47J::Set(Report,TEXT("expected_restore_state"),Expected);
 S47J::Set(Report,TEXT("ordinary_motel_drive_metres"),DriveMetres);S47J::Set(Report,TEXT("ordinary_motel_drive"),bDrove);
 S47J::Set(Report,TEXT("method"),bActual?TEXT("Actual SC09 PASS/checkpoint, ordinary case1 disk/title/Continue; ordinary World truck trip and native exterior/room Walker; actual camera-ray/HUD uses; offered talk and rule-book callbacks; full source-preserving disk/Flow restores."):TEXT("Isolated chapter4/prior-evidence fixture and one exterior Room6 start; ordinary door entry; continuous native Walker inside; actual camera-ray/HUD uses; offered talk and rule-book callbacks; fresh disk readers/full Flow restores and ordinary title Continue."));
 S47J::Set(Report,TEXT("limits"),bActual?TEXT("SC01-SC09 are consumed from exact named predecessor bytes, not replayed or fabricated. Ordinary trip uses API autopilot, Walker and dialogue callbacks are API-driven, not human input. No Nora recording played. PNG integrity is not visual or performance acceptance. SC11 still must drive from the motel to SARO."):TEXT("SC01-SC09 and the highway drive are prerequisites supplied by the developer fixture, not replayed. API-driven input, not human input. No Nora recording played. PNG integrity is not visual or performance acceptance."));
 Report->SetArrayField(TEXT("executed_checks"),Executed); Report->SetArrayField(TEXT("restores"),Cohorts);
 Report->SetArrayField(TEXT("rays"),Rays); Report->SetArrayField(TEXT("photos"),Photos);
 const bool Written = !Root.IsEmpty() && FFileHelper::SaveStringToFile(S47J::Write(Report,true),*(Root/TEXT("result.json")));
 if(Good && Written){UE_LOG(LogTemp,Display,TEXT("S47: spine contract sc10 PASS COMPLETE: %d checks, %d stores, %d actual uses, %.3fm, normalContinue=PASS evidence=%s"),Checks,Stores,Uses,Walked,*Root);}
 else {UE_LOG(LogTemp,Error,TEXT("S47: spine contract sc10 FAIL stage=%d checks=%d: %s (result written %d)"),Stage,Checks,*Why,Written);}
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

TMap<FString,FString> FS47SC10Contract::Documents() const
{
 TMap<FString,FString> Out;
 for(const auto& V:S47J::Arr(M->Story->SaveRoutine(),TEXT("docs")))
 {
  if(V&&V->Type==EJson::Object)
  {
   const auto D=V->AsObject();
   Out.Add(S47J::Str(D,TEXT("id")),S47J::Write(D));
  }
 }
 return Out;
}
bool FS47SC10Contract::Papers(const TMap<FString,FString>& Want,const FString& Context)
{
 const auto Now=Documents();for(const auto& D:Want)if(!Check(Now.Contains(D.Key)&&Now[D.Key]==D.Value,Context+TEXT(" complete paper/image/stamp/sheets: ")+D.Key))return false;return true;
}
bool FS47SC10Contract::Segment(const FVector2D& A,const FVector2D& B) const
{
 const auto* Player=M->Player.Get();const double Radius=Player->Radius;const FVector2D Delta=B-A;
 for(const auto& Box:M->Web->Colliders())
 {
  if(!Box.bEnabled||(Box.Id==TEXT("truck")&&Player->TruckBox&&Player->TruckBox->Area==M->World->Area)
   ||(Box.Id==TEXT("personalCar")&&Player->PersonalCarBox&&Player->PersonalCarBox->Area==M->World->Area))continue;
  double Lo=0,Hi=1;auto Clip=[&](double Pos,double D,double Low,double High){if(FMath::Abs(D)<1e-12)return Pos<Low||Pos>High;double U=(Low-Pos)/D,V=(High-Pos)/D;if(U>V)Swap(U,V);Lo=FMath::Max(Lo,U);Hi=FMath::Min(Hi,V);return Lo>Hi;};
  if(!Clip(A.X,Delta.X,Box.MinX-Radius,Box.MaxX+Radius)&&!Clip(A.Y,Delta.Y,Box.MinZ-Radius,Box.MaxZ+Radius))return false;
 }
 auto InCar=[&](const FVector2D& P){if(!Player->PersonalCarBox||Player->PersonalCarBox->Area!=M->World->Area)return false;const auto& Box=*Player->PersonalCarBox;
  const double Cs=FMath::Cos(Box.Heading),Sn=FMath::Sin(Box.Heading),X=P.X-Box.X,Z=P.Y-Box.Z;
  const double Ox=FMath::Max(0.,FMath::Abs(X*Cs-Z*Sn)-Box.HalfW),Oz=FMath::Max(0.,FMath::Abs(X*Sn+Z*Cs)-Box.HalfL);return Ox*Ox+Oz*Oz<Radius*Radius;};
 const int32 Count=FMath::Max(1,FMath::CeilToInt(FVector2D::Distance(A,B)/.04));
 for(int32 Index=0;Index<=Count;++Index){const auto P=FMath::Lerp(A,B,double(Index)/Count);if(!Player->Walkable(P.X,P.Y)||(Player->TruckBox&&Player->TruckBox->Area==M->World->Area&&Player->InTruck(P.X,P.Y))||InCar(P))return false;}return true;
}
bool FS47SC10Contract::Go(const FString& Id)
{
 const auto* Item=M->Inter->Get(Id);const auto* Exported=M->Web->Item(Id);const auto Obj=Item?Item->Object:Exported?M->Web->ObjAt(Exported->Node):FS47Obj();const auto Bounds=Obj.WorldBox();
 if(!Check(Obj&&Bounds.IsValid,TEXT("original world interaction bounds for native approach: ")+Id))return false;
 const FVector At=Bounds.GetCenter();const double Range=Item&&Item->Range>0?Item->Range:Exported&&Exported->Range>0?Exported->Range:2.4;
 TArray<FS47Rect> Blocks=M->Web->Colliders();Blocks.RemoveAll([this](const FS47Rect& B){return(B.Id==TEXT("truck")&&M->Player->TruckBox&&M->Player->TruckBox->Area==M->World->Area)
  ||(B.Id==TEXT("personalCar")&&M->Player->PersonalCarBox&&M->Player->PersonalCarBox->Area==M->World->Area);});
 const TArray<FVector2D> Seeds={{5.5,1.6},{6.5,1.6},{7.5,1.6},{9.95,1.6},{9.95,3.7},{-33,656},{-37,663},{-38.5,672},{-39.5,685},{-40,699},{-42,705}};
 for(double Radius:{.8,1.2,1.6,2.0})for(int32 Index=0;Index<24;++Index)
 {
  const FVector2D Goal(At.X+Radius*FMath::Cos(Index*PI/12),At.Z+Radius*FMath::Sin(Index*PI/12));if(!M->Player->Walkable(Goal.X,Goal.Y))continue;
  double Floor=M->Player->Floor?M->Player->Floor(Goal.X,Goal.Y):M->Player->FloorY;if(!FMath::IsFinite(Floor))Floor=M->Player->FloorY;
  const FVector Eye(Goal.X,Floor+M->Player->Eye,Goal.Y),D=At-Eye;if(D.Size()>Range-.04||M->Inter->Pick(S47Three::ToUE(Eye),S47Three::DirToUE(D.GetSafeNormal()))!=Id)continue;
  auto Points=FS47ThreatNavigation::FindPath({M->Player->Pos.X,M->Player->Pos.Z},Goal,Seeds,Blocks,M->Player->Radius,[this](const FVector2D& A,const FVector2D& B){return Segment(A,B);});
  if(Points.IsEmpty())continue;Path(MoveTemp(Points));return true;
 }
 Result(false,TEXT("no bounded legal native approach and actual ray to ")+Id);return false;
}

bool FS47SC10Contract::Restore(const FString& Name)
{
 M->UI->Close(true);
 if(bActual)
 {
  if(M->Story->IsCinematic()||M->World->bBusy||!M->Flow->GameSaveBlock().IsEmpty())return false;
  if(!Check(M->Flow->CaseId==1&&!M->Flow->bDevRun,TEXT("ordinary isolated case1 save boundary: ")+Name)||!Papers(PriorDocs,Name))return false;
  ExpectedDocs=Documents();Expected=S47J::Clone(M->Flow->GameState());FString Error;
  const auto Meta=M->Flow->WriteSave(TEXT("manual"),0,Error);FS47SaveStore Reader(M->Flow->Saves.Dir);Reader.Init();const auto Disk=Meta?Reader.Load(Meta->Id):nullptr;
  if(!Check(Disk&&SC10Same(Disk,Expected)&&FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/(Name+TEXT(".disk.json")))),TEXT("ordinary manual plus independent complete reader: ")+Name+Error))return false;
  if(Name==TEXT("sc10-complete")&&!Check(FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(Root/TEXT("sc10-complete.disk.json"))),TEXT("stable actual SC10 output for SC11 before return travel")))return false;
  M->Flow->ToTitle();FS47SaveStore TitleReader(M->Flow->Saves.Dir);TitleReader.Init();const auto Selected=TitleReader.ContinueSave(),FlowSelected=M->Flow->Saves.ContinueSave();const auto Chosen=Selected?TitleReader.Load(Selected->Id):nullptr;
  if(!Check(M->Mode==US47Main::EMode::Title&&Selected&&FlowSelected&&Selected->Id==FlowSelected->Id&&SC10Same(Chosen,Disk),TEXT("ordinary title/fresh reader pins the same complete disk state: ")+Name))return false;
  if(!Check(M->Flow->OpenTestView(TEXT("continue")),TEXT("ordinary title Continue loads actual case1: ")+Name))return false;
  PendingRestore=Name;++Stores;bNormalContinue=true;return true;
 }
 Expected = S47J::Clone(M->Flow->GameState());
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
 if(bActual)
 {
  auto Raw=S47J::New();S47J::Set(Raw,TEXT("name"),PendingRestore);S47J::Set(Raw,TEXT("expected_full_state"),Expected);S47J::Set(Raw,TEXT("actual_full_state"),Now);Cohorts.Add(MakeShared<FJsonValueObject>(Raw));
  for(const TCHAR* Key:{TEXT("office"),TEXT("spine"),TEXT("horror"),TEXT("truck"),TEXT("odo")})if(!Check(SC10Same(S47J::Obj(Now,Key),S47J::Obj(Expected,Key)),FString(TEXT("actual full Flow retains world/source state "))+Key+TEXT(" / ")+PendingRestore))return false;
  if(!Check(M->Flow->CaseId==1&&!M->Flow->bDevRun,TEXT("actual Continue remains ordinary case1"))||!Papers(ExpectedDocs,PendingRestore)||!Papers(PriorDocs,PendingRestore))return false;
 }
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
 if(bActual&&Distance>.002)ProgressAt=FPlatformTime::Seconds();
 if(bActual&&(!Player->Walkable(Player->Pos.X,Player->Pos.Z)||!Route.IsValidIndex(Waypoint)||!Segment({Player->Pos.X,Player->Pos.Z},Route[Waypoint])))
 {Result(false,TEXT("actual Walker/route became blocked; no placement or door recovery"));return false;}
 if (FPlatformTime::Seconds()-ProgressAt > 18) { Result(false,TEXT("physical room route stalled; no placement recovery allowed")); return false; }
 const FVector2D Delta = Route[Waypoint]-FVector2D(Player->Pos.X,Player->Pos.Z);
 if (Delta.Size() < (bActual?.012:.09))
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
  Started = true; Began = FPlatformTime::Seconds(); FString UserDir, Jump,Input;
  FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir); FParse::Value(FCommandLine::Get(),TEXT("S47Jump="),Jump);
  bActual=FParse::Value(FCommandLine::Get(),TEXT("S47SC09Checkpoint="),Input);
  UserDir = FPaths::ConvertRelativePathToFull(UserDir); FPaths::CollapseRelativeDirectories(UserDir);
  if (!Check(M->Flow->bDevRun && M->Flow->CaseId == 0 && (bActual?Jump.IsEmpty():Jump==TEXT("chapter4")) && (UserDir.StartsWith(TEXT("/tmp/")) || UserDir.StartsWith(TEXT("/var/tmp/")))
   && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),UserDir)
   && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),UserDir),TEXT("explicit isolated case0 chapter4 fixture; no player save"))) return;
  Root = FPaths::ProjectSavedDir()/TEXT("SC10Contract")/FGuid::NewGuid().ToString(EGuidFormats::Digits); IFileManager::Get().MakeDirectory(*Root,true);
  if (!Check(M->Flow->Saves.List().IsEmpty(),TEXT("initial isolated profile has no saved cases"))) return;
  if(bActual)
  {
   for(const TCHAR* Flag:{TEXT("S47Drive"),TEXT("S47Use"),TEXT("S47Press"),TEXT("S47Open"),TEXT("S47WalkFrom"),TEXT("S47Walk"),TEXT("S47Face"),TEXT("S47Auto"),TEXT("Clock"),TEXT("S47Clock"),TEXT("S47SpineStart"),TEXT("S47EveningRoute"),TEXT("S47ProductionViews"),TEXT("S47Ch4Drive"),TEXT("S47MotelTv"),TEXT("S47MotelV2Route"),TEXT("S47HorrorContract"),TEXT("S47NightSequenceContract")})
   {FString Value;if(FParse::Param(FCommandLine::Get(),Flag)||FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),Value)){Result(false,TEXT("competing world/fixture driver: ")+FString(Flag));return;}}
   const FString ReceiptPath=FPaths::GetPath(Input)/TEXT("result.json");FString Data,ReceiptData;
   const bool Read=!FPaths::IsRelative(Input)&&FPaths::GetCleanFilename(Input)==TEXT("sc09-complete.disk.json")&&FFileHelper::LoadFileToString(Data,*Input)&&FFileHelper::LoadFileToString(ReceiptData,*ReceiptPath);
   PriorState=Read?S47J::Read(Data):nullptr;const auto Receipt=Read?S47J::Read(ReceiptData):nullptr;
   const auto InputFile=SC10File(Input),Certified=S47J::Obj(Receipt,TEXT("checkpoint_file"));
   if(!Check(PriorState&&Receipt&&InputFile&&Certified&&S47J::Str(Receipt,TEXT("status"))==TEXT("PASS")&&S47J::Str(Receipt,TEXT("scenario"))==TEXT("SC09")&&S47J::Str(Receipt,TEXT("mode"))==TEXT("actual-checkpoint")
    &&S47J::Str(Certified,TEXT("path"))==Input&&S47J::Num(Certified,TEXT("bytes"))==S47J::Num(InputFile,TEXT("bytes"))&&S47J::Str(Certified,TEXT("sha1"))==S47J::Str(InputFile,TEXT("sha1")),TEXT("actual SC09 PASS binds this exact checkpoint path/bytes/SHA1; prior fixture rejected")))return;
   ChainId=S47J::Str(Receipt,TEXT("chain_id"));const auto Case=S47J::Obj(PriorState,TEXT("case")),Spine=S47J::Obj(PriorState,TEXT("spine")),Ch3=S47J::Obj(Case,TEXT("ch3")),Ch4=S47J::Obj(Case,TEXT("ch4"));
   if(!Check(!ChainId.IsEmpty()&&S47J::Int(PriorState,TEXT("timelineVersion"))==2&&(S47J::Int(Spine,TEXT("v"))==1||S47J::Int(Spine,TEXT("v"))==2)
    &&S47J::Int(Spine,TEXT("sc08"))==2&&S47J::Int(Spine,TEXT("sc11"))==0&&S47J::Str(PriorState,TEXT("checkpoint"))==TEXT("chapter4")
    &&S47J::Str(Ch3,TEXT("stage"))==TEXT("complete")&&S47J::Int(Ch3,TEXT("surveyVersion"))==1&&S47J::Int(Ch3,TEXT("dev"))==3&&S47J::Bool(Ch3,TEXT("called"))
    &&S47J::Bool(Ch3,TEXT("p06"))&&S47J::Bool(Ch3,TEXT("p07"))&&S47J::Bool(Ch3,TEXT("p08"))&&S47J::Bool(Ch3,TEXT("p09"))
    &&S47J::Str(Ch4,TEXT("stage"))==TEXT("to-motel")&&!S47J::Bool(Ch4,TEXT("arrived"))&&!S47J::Bool(Ch4,TEXT("greeted"))
    &&S47J::Num(Case,TEXT("clock"))>=S47Timeline::Chapter4(true),TEXT("real secured SC09/Nora and untouched04:50 SC10 boundary, compatible spine1/2")))return;
   if(!Check(SC10Copy(Input,Root/TEXT("input-sc09.disk.json"))&&SC10Copy(ReceiptPath,Root/TEXT("input-sc09.result.json")),TEXT("byte-identical SC09 checkpoint and PASS receipt retained")))return;
   FString CopiedData,CopiedReceipt;const auto Copied=SC10File(Root/TEXT("input-sc09.disk.json"));
   if(!Check(Copied&&S47J::Str(Copied,TEXT("sha1"))==S47J::Str(InputFile,TEXT("sha1"))&&S47J::Num(Copied,TEXT("bytes"))==S47J::Num(InputFile,TEXT("bytes"))&&SC10Same(SC10File(Input),InputFile)
    &&FFileHelper::LoadFileToString(CopiedData,*(Root/TEXT("input-sc09.disk.json")))&&FFileHelper::LoadFileToString(CopiedReceipt,*(Root/TEXT("input-sc09.result.json")))&&SC10Same(S47J::Read(CopiedData),PriorState)&&SC10Same(S47J::Read(CopiedReceipt),Receipt),TEXT("copied source bytes rehash and parse to the certified actual input")))return;
   Prerequisite=S47J::New();S47J::Set(Prerequisite,TEXT("checkpoint"),SC10File(Input));S47J::Set(Prerequisite,TEXT("result"),SC10File(ReceiptPath));
   US47Flow::FStart Start;Start.State=PriorState;Start.CaseId=1;Start.bDev=false;M->Flow->StartGame(Start);Next(100,.5);return;
  }
  US47Chapter4* Part = Chapter();
  if (!Check(Part && Part->bActive && M->Spine->Enabled() && M->Story->bEveningTimeline && M->Story->Clock() >= S47Timeline::Chapter4(true)
   && M->Story->Clock() < 5*3600 && S47J::Num(Part->Ch3(),TEXT("dev")) >= 3 && S47J::Bool(Part->Ch3(),TEXT("called")),TEXT("04:50 SC10 fixture retains both preceding secured field prints and completed Nora call"))) return;
  for (const TCHAR* Flag : {TEXT("S47Ch4Drive"),TEXT("S47MotelTv"),TEXT("S47MotelV2Route"),TEXT("S47HorrorContract")})
  { FString Value; if (FParse::Param(FCommandLine::Get(),Flag) || FParse::Value(FCommandLine::Get(),*(FString(Flag)+TEXT("=")),Value)) { Result(false,TEXT("independent driver also enabled")); return; } }
  const FS47Pose Outside = Part->Court->Anchor(TEXT("room6Outside"));
  if (!Check(Outside.bValid && M->Player->Walkable(Outside.X,Outside.Z),TEXT("one legal exterior fixture at existing room6Outside anchor"))) return;
  M->Player->Place(Outside.X,Outside.Z,Outside.Yaw); Next(1,.5); return;
 }
 if (FPlatformTime::Seconds()-Began > (bActual?600:180)) { Result(false,TEXT("bounded SC10 route timeout")); return; }
 if (M->Flow->bStarting || S47Startup::Blocking() || M->World->bBusy) return;
 if (Hold > 0) { Hold -= Dt; return; }
 if (!PendingRestore.IsEmpty() && !Restored()) return;
 US47Chapter4* Part = Chapter(); if (!Part) { Result(false,TEXT("SC10 owner disappeared")); return; }
 const auto Talk = Part->Room->Anchor(TEXT("talk")), Arrive = Part->Room->Anchor(TEXT("arrive")), Exit = Part->Room->Anchor(TEXT("exit"));
 const double Ox = Arrive.X+1.4, Oz = Arrive.Z-4.3;
 switch (Stage)
 {
 case 100:
 {
  const auto Now=M->Flow->GameState();
  if(!Check(Part->bActive&&Part->S.Stage==TEXT("to-motel")&&!Part->S.bArrived&&M->Story->ChapterKey==TEXT("ch4")&&M->Spine->Enabled()&&M->Flow->CaseId==1&&!M->Flow->bDevRun,TEXT("actual SC09 checkpoint restored into ordinary isolated case1")))return;
  for(const TCHAR* Key:{TEXT("s"),TEXT("ch2"),TEXT("ch3"),TEXT("ch4")})if(!Check(SC10Same(S47J::Obj(S47J::Obj(Now,TEXT("case")),Key),S47J::Obj(S47J::Obj(PriorState,TEXT("case")),Key)),FString(TEXT("inherited actual source state unchanged: "))+Key))return;
  const auto Case=S47J::Obj(PriorState,TEXT("case"));
  for(const TCHAR* Id:{TEXT("frame01"),TEXT("frame02"),TEXT("frame03"),TEXT("frame04")})
  {
   const FString PhotoId(Id);const auto Owner=S47J::Obj(Case,(PhotoId==TEXT("frame01")||PhotoId==TEXT("frame02"))?TEXT("s"):TEXT("ch3"));
   const TCHAR* Key=PhotoId==TEXT("frame01")?TEXT("f1"):PhotoId==TEXT("frame02")?TEXT("f2"):PhotoId==TEXT("frame03")?TEXT("f3"):TEXT("f4");
   const FString Url=S47J::Str(S47J::Obj(Owner,Key),TEXT("url"));const auto* D=M->Story->Docs().FindByPredicate([Id](const FS47Doc& V){return V.Id==Id;});
   if(!Check(D&&Url.StartsWith(TEXT("data:image/jpeg;base64,"))&&D->Image==Url,FString(TEXT("actual inherited photograph payload: "))+Id))return;
  }
  for(const TCHAR* Id:{TEXT("obs_runbinder"),TEXT("e07"),TEXT("e10"),TEXT("EV_DALE_INDEX"),TEXT("EV_DALE_NOTE")})if(!Check(M->Story->Docs().ContainsByPredicate([Id](const FS47Doc& D){return D.Id==Id;}),FString(TEXT("actual inherited paper, no factory substitute: "))+Id))return;
  PriorDocs=Documents();if(Restore(TEXT("actual-sc09-input")))Next(101,.5);return;
 }
 case 101:
  if(M->World->AtMotel(M->Player->Pos)){Next(116);return;}
  if(!Check(M->World->Area==TEXT("saro")&&M->World->TruckAt==TEXT("saro"),TEXT("actual service truck remains at SARO for the ordinary motel trip")))return;
  if(!M->Web->IsDoorOpen(TEXT("doorEast"))){if(Go(TEXT("doorEast")))Next(110,0);}else Next(112,0);return;
 case 110:if(Walk()){Face(TEXT("doorEast"));Next(111,.4);}return;
 case 111:if(Use(TEXT("doorEast")))Next(112,.7);return;
 case 112:if(Go(TEXT("truck")))Next(113,0);return;
 case 113:if(Walk()){Face(TEXT("truck"));Next(114,.4);}return;
 case 114:DriveOdo=M->World->Odometer();M->World->Autopilot=24;if(Use(TEXT("truck")))Next(115,0);return;
 case 115:
  bDrove|=M->World->bDriving;if(M->World->bDriving)return;
  DriveMetres=(M->World->Odometer()-DriveOdo)*1609.344;
  if(!Check(bDrove&&DriveMetres>100&&M->World->Area==TEXT("saro")&&M->World->TruckAt==TEXT("motel")&&M->World->AtMotel(M->Player->Pos),TEXT("ordinary World/autopilot motel trip actually parked without teleport")))return;Next(116,.5);return;
 case 116:if(Go(TEXT("room6Door")))Next(117,0);return;
 case 117:if(Walk()){Face(TEXT("room6Door"));Next(2,.4);}return;
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
  {
   const auto Case=S47J::Obj(M->Flow->GameState(),TEXT("case"));const FString Method=S47J::Str(S47J::Obj(Case,TEXT("s")),TEXT("method"));
   if(!Check(Method==TEXT("passive")||Method==TEXT("active"),TEXT("P10 reads the actual retained B-12 control method")))return;
   if(!Choice(TEXT("receiver"))||!Check(!Part->S.bP10&&Part->Reply.IsSet(),TEXT("wrong control remains unanswered"))||!Choice(Method==TEXT("active")?TEXT("none"):TEXT("lamp")))return;
  }
  if (!Check(Part->S.bP10,TEXT("the actual active/passive print controls the accepted answer")) || !Choice(TEXT("tomas")) || !Choice(TEXT("record")) || !Choice(TEXT("critical"))) return;
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
  if(!bActual){M->Flow->Saves = FS47SaveStore(Root/TEXT("normal-save")); M->Flow->Saves.Init(); M->Flow->CaseId = 1; M->Flow->bDevRun = false; M->Flow->Saves.SetActive(1);}
  Next(24,.4); return;
 case 24:
 {
  if (!M->Flow->SaveBlock().IsEmpty()) { if (FPlatformTime::Seconds()-Began > (bActual?580:160)) Result(false,TEXT("ordinary save gate: ")+M->Flow->SaveBlock()); return; }
  if(bActual){if(!Papers(PriorDocs,TEXT("before-normal-title")))return;ExpectedDocs=Documents();}
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
  if(bActual)
  {
   const auto State=M->Flow->GameState();const auto Ch4=S47J::Obj(S47J::Obj(State,TEXT("case")),TEXT("ch4"));
   if(!Check(S47J::Str(State,TEXT("checkpoint"))==TEXT("chapter4")&&S47J::Str(Ch4,TEXT("stage"))==TEXT("complete")&&S47J::Bool(Ch4,TEXT("told"))&&S47J::Bool(Ch4,TEXT("p10"))&&S47J::Bool(Ch4,TEXT("p11"))&&S47J::Bool(Ch4,TEXT("p12")),TEXT("actual stable C4 completion before SC11 return trip")))return;
   if(Restore(TEXT("sc10-complete")))Next(118,.4);return;
  }
  if (!Check(Stores == 6 && Uses == 8 && Photos.Num() == 3 && Walked > 10 && WalkSamples > 20,TEXT("all six disk cohorts, eight ray uses, three pictures and continuous room route completed"))) return;
  if(!Check(SC10Copy(Root/TEXT("complete-before-transition.disk.json"),Root/TEXT("sc10-complete.disk.json")),TEXT("named fixture output preserves its actual disk bytes, not a chained prior playthrough")))return;
  Result(true,TEXT("SC10 full Room6 evidence/conversation/physical entry-exit and disk continuation")); return;
 case 118:
  if(!Check(M->Spine->Gate(FS47Spine::SC11)==1&&!M->Story->Begun.Contains(TEXT("ch5"))&&M->Flow->CaseId==1&&!M->Flow->bDevRun
   &&Stores>=8&&Uses>=(bDrove?9:8)&&Photos.Num()==3&&Walked>10&&WalkSamples>20,TEXT("actual case1 source chain, ordinary motel/room route and stable SC11-pending disk/title Continue")))return;
  Result(true,TEXT("actual SC09-to-SC10 chain; ordinary motel trip, Room6 evidence and stable SC11 handoff"));return;
 default: Result(false,TEXT("unknown SC10 test phase")); return;
 }
}

void S47StepSC10Contract(US47Main* Main, double Dt)
{
 static TWeakObjectPtr<US47Main> Owner; static TSharedPtr<FS47SC10Contract> Test;
 if (Owner.Get() != Main) { Owner = Main; Test = MakeShared<FS47SC10Contract>(Main); }
 const auto KeepAlive=Test;if(KeepAlive)KeepAlive->Step(Dt);
}

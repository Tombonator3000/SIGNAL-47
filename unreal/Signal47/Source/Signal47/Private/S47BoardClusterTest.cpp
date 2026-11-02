#include "S47BoardClusterTest.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Chapter1.h"
#include "S47CaseBoard.h"
#include "SS47Board.h"
#include "S47Flow.h"
#include "S47UI.h"
#include "S47Saves.h"
#include "Containers/Ticker.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"

namespace
{
struct FRun
{
 TWeakObjectPtr<US47Main> Main;
 FString Root,Photo1,Photo2; int32 Stage=0,Checks=0,Restores=0,Threads=0; double Age=0,At=0;
 bool OldLarge=false,bStarted=false; FVector2D Manual; TOptional<FVector2D> Unrelated;
 S47J::FObj Layout; TMap<FString,FVector4> Bounds; TArray<FString> Links; TArray<TSharedPtr<FJsonValue>> Receipts;
 TArray<FString> Shots;
 bool Finish(bool Good,const FString& Why)
 {
  if(auto* M=Main.Get();bStarted&&M&&M->Flow)M->Flow->Settings().OnLargeText(OldLarge);
  auto O=S47J::New(); S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL")); S47J::Set(O,TEXT("reason"),Why);
  S47J::Set(O,TEXT("checks"),Checks);S47J::Set(O,TEXT("diskRestores"),Restores);S47J::Set(O,TEXT("stage"),Stage);
  S47J::Set(O,TEXT("provenance"),TEXT("Native Slate/API contract using existing Ch1Drive photo journey and its declared chapter-state shortcuts. Mark2 is an explicit fixture at the actual second photograph's vane target; manual card position is a fixture. Real board Link, Settings callback, WriteSave, fresh disk reader and full Flow OpenSave. No ordinary OS input, natural photo-marking, subjective readability or overlap acceptance claimed."));
  O->SetArrayField(TEXT("receipts"),Receipts);S47J::Set(O,TEXT("screenshots"),Shots);
  if(!Root.IsEmpty()&&!FFileHelper::SaveStringToFile(S47J::Write(O,true),*(Root/TEXT("result.json"))))Good=false;
  UE_LOG(LogTemp,Display,TEXT("S47 board clusters %s checks=%d diskRestores=%d: %s"),Good?TEXT("PASS"):TEXT("FAIL"),Checks,Restores,*Why);
  FPlatformMisc::RequestExitWithStatus(false,Good?0:1);return false;
 }
 bool Check(bool Good,const FString& Why)
 {
  ++Checks;auto O=S47J::New();S47J::Set(O,TEXT("pass"),Good);S47J::Set(O,TEXT("check"),Why);Receipts.Add(MakeShared<FJsonValueObject>(O));
  if(!Good)Finish(false,Why);return Good;
 }
 void Next(int32 N){Stage=N;At=Age;}
 void Shot(const TCHAR* Name){const FString P=Root/(FString(Name)+TEXT(".png"));Shots.Add(P);FScreenshotRequest::RequestScreenshot(P,true,false);}
 bool Same(const TOptional<FVector2D>& A,const TOptional<FVector2D>& B)const{return A.IsSet()==B.IsSet()&&(!A||A->Equals(*B,.001));}
 bool Equal(const S47J::FObj& A,const S47J::FObj& B)const{return FJsonValue::CompareEqual(FJsonValueObject(A),FJsonValueObject(B));}
 bool Tick(float Dt)
 {
  Age+=Dt;auto* M=Main.Get();if(!M||Age>200)return Finish(false,TEXT("main unavailable or 200 second timeout"));
  if(Age-At<.6)return true;
  auto* C=Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")));auto* B=M->CaseBoard.Get();
  if(Stage==0)
  {
   if(!C||!C->S.F1||!C->S.F2||!B||!B->View.IsValid())return true;
   const auto* Vane=C->S.F2->Targets.FindByPredicate([](const auto& T){return T.Key==TEXT("vane");});
   if(!Check(Vane&&C->S.Dev1==3&&C->S.Dev2==3&&!C->S.F1->Url.IsEmpty()&&!C->S.F2->Url.IsEmpty(),TEXT("existing photo driver produced two developed source photographs and a real vane target")))return false;
   C->S.Mark2=Vane->Value;B->Refresh();B->View->Look(TEXT("ch1"));B->View->HideHow();
   const auto Anchor=B->GetPosition(TEXT("frame01"));
   if(!Check(Anchor.IsSet(),TEXT("actual existing linked frame01 has a layout position")))return false;
   Manual=*Anchor+FVector2D(24,0);B->SetPosition(TEXT("frame01"),Manual.X,Manual.Y);B->Refresh();
   Unrelated=B->GetPosition(TEXT("s03log"));Threads=C->Book()->Threads().Num();Photo1=C->S.F1->Url;Photo2=C->S.F2->Url;
   const auto Cards=C->Book()->Cards();bool Have=true;
   for(const TCHAR* Id:{TEXT("frame01"),TEXT("frame02"),TEXT("observation")})Have&=Cards.ContainsByPredicate([&](const auto& Card){return Card.Id==Id&&Card.bHave;});
   if(!Check(Have&&B->Rests(TEXT("frame01.extra"))&&B->Rests(TEXT("frame02.extra"))&&B->Rests(TEXT("observation.stripe")),TEXT("all three actual cards are collected and expose the tested pins")))return false;
   Next(1);return true;
  }
  if(Stage==1){Shot(TEXT("01-before-links"));Next(2);return true;}
  if(Stage==2)
  {
   const auto R=B->View->Link(TEXT("frame01.extra"),TEXT("frame02.extra"));
   if(!Check(R.bOk&&C->Book()->Threads().Num()==Threads+1,TEXT("first new thread links the two actual photograph cards")))return false;
   if(!Check(Same(B->GetPosition(TEXT("frame01")),Manual)&&Same(B->GetPosition(TEXT("s03log")),Unrelated),TEXT("two-card link preserves manual anchor and unrelated card position")))return false;
   Next(3);return true;
  }
  if(Stage==3){Shot(TEXT("02-two-cards"));Next(4);return true;}
  if(Stage==4)
  {
   const auto R=B->View->Link(TEXT("observation.stripe"),TEXT("frame02.detail"));
   if(!Check(R.bOk&&C->Book()->Threads().Num()==Threads+2&&C->Book()->Filled(TEXT("P03")).Num()==2,TEXT("second new thread grows the connected evidence to three cards and fills P03")))return false;
   if(!Check(Same(B->GetPosition(TEXT("frame01")),Manual)&&Same(B->GetPosition(TEXT("s03log")),Unrelated),TEXT("three-card growth preserves manual and unrelated positions")))return false;
   const auto Before=B->Save();const auto Bad=B->View->Link(TEXT("frame02.extra"),TEXT("s03log.zero"));
   if(!Check(!Bad.bOk&&C->Book()->Threads().Num()==Threads+2&&Equal(Before,B->Save()),TEXT("rejected real connection adds no edge and moves no card")))return false;
   Next(5);return true;
  }
  if(Stage==5){Shot(TEXT("03-three-cards"));Next(6);return true;}
  if(Stage==6)
  {
   const auto Before=B->Save();B->View->ShowAll();const FVector Fit=B->View->View();
   auto Key=[&](FKey K){B->View->OnKeyDown(B->View->GetCachedGeometry(),FKeyEvent(K,FModifierKeysState(),0,false,0,0));};
   Key(EKeys::Gamepad_RightShoulder);Key(EKeys::Gamepad_RightShoulder);const FVector Zoom=B->View->View();
   Key(EKeys::Gamepad_DPad_Right);Key(EKeys::Gamepad_DPad_Down);const FVector Pan=B->View->View();
   if(!Check(Zoom.X>Fit.X&&(Pan.Y!=Zoom.Y||Pan.Z!=Zoom.Z)&&Equal(Before,B->Save()),TEXT("real Slate key handlers zoom and pan without changing card layout")))return false;
   M->UI->Close(true);M->Flow->Settings().OnLargeText(true);B->Open(TEXT("CLUSTER CONTRACT"),TEXT("ch1"));B->View->HideHow();Next(7);return true;
  }
  if(Stage==7)
  {
   if(!Check(M->UI->bLargeText&&Same(B->GetPosition(TEXT("frame01")),Manual)&&C->Book()->Threads().Num()==Threads+2,TEXT("Settings large text and actual reopened widget preserve manual anchor and links")))return false;
   Shot(TEXT("04-large-text"));Next(8);return true;
  }
  if(Stage==8)
  {
   Bounds=B->View->LayoutBounds();Layout=S47J::Clone(B->Save());Links=*C->S.Links;M->UI->Close(true);FString Error;
   if(!Check(M->Flow->SaveBlock().IsEmpty(),TEXT("ordinary save is available in the isolated case")))return false;
   const auto Meta=M->Flow->WriteSave(TEXT("manual"),0,Error);
   if(!Check(Meta.IsSet(),TEXT("ordinary WriteSave writes manual slot: ")+Error))return false;
   FS47SaveStore Reader;Reader.Init();const auto Disk=Reader.Load(Meta->Id);const auto Continue=Reader.ContinueSave();
   if(!Check(Disk.IsValid()&&Continue&&Continue->Id==Meta->Id,TEXT("fresh disk reader and Continue resolve the newly written save")))return false;
   M->Flow->OpenSave(*Continue);++Restores;Next(9);return true;
  }
  if(Stage==9)
  {
   if(M->Flow->bStarting)return true;
   if(!Check(C&&C->S.F1&&C->S.F2&&C->S.Links&&C->S.F1->Url==Photo1&&C->S.F2->Url==Photo2&&*C->S.Links==Links,TEXT("full disk/Flow return preserves both source photographs and all real links")))return false;
   if(!Check(Equal(B->Save(),Layout),TEXT("full Flow return restores every saved position and automatic/manual ownership")))return false;
   B->Open(TEXT("RESTORED CLUSTER"),TEXT("ch1"));B->View->HideHow();Next(10);return true;
  }
  if(Stage==10)
  {
   if(!Check(Same(B->GetPosition(TEXT("frame01")),Manual)&&C->Book()->Threads().Num()==Threads+2,TEXT("restored actual board retains manual anchor and all thread endpoints")))return false;
   const auto Actual=B->View->LayoutBounds();bool Stable=Actual.Num()==Bounds.Num();FString Difference;
   for(const auto& Pair:Bounds){const auto* Found=Actual.Find(Pair.Key);const bool Match=Found&&Found->Equals(Pair.Value,.01);Stable&=Match;if(!Match)Difference+=FString::Printf(TEXT(" %s: %s -> %s;"),*Pair.Key,*Pair.Value.ToString(),Found?*Found->ToString():TEXT("missing"));}
   if(!Check(M->UI->bLargeText&&Stable,TEXT("restored large-text widget preserves every measured card and Finding position and size")+Difference))return false;
   Shot(TEXT("05-restored"));Next(11);return true;
  }
  for(const auto& Path:Shots)if(!IFileManager::Get().FileExists(*Path))return true;
  return Finish(true,TEXT("native board growth, view controls, large text and one disk/Flow return complete"));
 }
};
}

void S47BoardClusterTest::Start(US47Main* M)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47BoardClusterContract")))return;
 auto R=MakeShared<FRun>();R->Main=M;FString User,Jump,Drive;
 FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);FParse::Value(FCommandLine::Get(),TEXT("S47Jump="),Jump);FParse::Value(FCommandLine::Get(),TEXT("S47Ch1Drive="),Drive);
 if(!M||!M->Flow||!User.StartsWith(TEXT("/tmp/s47-board-cluster-"))||User.Contains(TEXT(".."))||Jump!=TEXT("chapter1")||Drive!=TEXT("board")||M->Flow->CaseId!=1||FParse::Param(FCommandLine::Get(),TEXT("S47SmokeTest")))
 {R->Finish(false,TEXT("requires isolated /tmp/s47-board-cluster-* UserDir, S47Case=1, chapter1/board driver and no SmokeTest"));return;}
 R->OldLarge=M->UI->bLargeText;R->Root=FPaths::ProjectSavedDir()/TEXT("BoardClusterContract");IFileManager::Get().MakeDirectory(*R->Root,true);
 for(const auto& Meta:M->Flow->Saves.List())if(Meta.Kind==TEXT("manual")){R->Finish(false,TEXT("refusing an existing manual-save profile; use a fresh isolated UserDir"));return;}
 R->bStarted=true;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([R](float Dt){return R->Tick(Dt);}));
}

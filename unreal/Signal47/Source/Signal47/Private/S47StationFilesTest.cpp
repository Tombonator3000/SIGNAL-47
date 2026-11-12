#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Flow.h"
#include "S47Startup.h"
#include "S47StationFiles.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47World.h"
#include "UnrealClient.h"

// FIX-20261110 (Tom, points 2 and 3), -S47StationFilesTest: SARO's own files under OPS / RECORDS / STATION FILES and in the
// records binder: four on the first day (the generator memo and the operator's card among them), the rest released by day.
void FS47FreeformTest::StepStationFiles(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>90){Finish(false,FString::Printf(TEXT("station files exceeded the time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->World)return;
 auto& F=*M->Freeform;
 auto Shot=[&](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Root/Name,true,false);};
 // On the glass the file is a tab of OPS / RECORDS; off it, a page behind the binder's STATION FILES divider (RECORDS-BINDER).
 auto Show=[&](const TCHAR* Key,int32 Page){F.ArchiveTab=4;F.ArchivePick=Key;F.ArchivePage=Page;if(F.TerminalOnGlass())F.OpenArchive();else F.BinderShow(S47Binder::FileKey(Key),Page);};
 if(Stage==0)
 {
  if(Age<3)return;M->UI->Close(true);
  if(!Check(S47StationFiles::Lint().IsEmpty(),TEXT("the files keep to the spoiler limits: ")+S47StationFiles::Lint()))return;
  int32 First=0;for(int32 I=0;I<S47StationFiles::Count();++I)First+=S47StationFiles::Get(I).Day<=1;
  if(!Check(S47StationFiles::Count()==9&&First==4&&F.StationFileDue(),TEXT("nine files, four of them there on the first day and marked on RECORDS")))return;
  if(!Check(F.Use(TEXT("opsTerminal")),TEXT("the OPS terminal opens")))return;Go(1,.6);return;
 }
 if(Stage==1)
 {
  if(!F.TerminalWindowRendered())return;Show(TEXT("station-02"),0);
  if(!Check(F.ArchiveTab==4&&F.OperationsSection()==3&&F.TerminalOnGlass(),TEXT("STATION FILES is a tab of OPS / RECORDS on the glass")))return;Go(2,.7);return;
 }
 if(Stage==2){Shot(TEXT("station-files-generator.png"));Go(3,.3);return;}
 if(Stage==3){Show(TEXT("station-08"),0);Go(4,.7);return;}
 if(Stage==4){Shot(TEXT("station-files-card.png"));Go(5,.3);return;}
 if(Stage==5)
 {
  // Which of the first day's files run to a second page depends on the glass; one of them must, and it must turn.
  bool Turned=false;for(const TCHAR* Key:{TEXT("station-01"),TEXT("station-04"),TEXT("station-02"),TEXT("station-08")}){Show(Key,1);if(F.ArchivePage==1){Turned=true;break;}}
  if(!Check(Turned,TEXT("a long file turns to its second page")))return;Go(6,.7);return;
 }
 if(Stage==6){Shot(TEXT("station-files-card-2.png"));Go(7,.3);return;}
 if(Stage==7)
 {
  Show(TEXT("station-01"),0);Show(TEXT("station-04"),0);
  if(!Check(!F.StationFileDue(),TEXT("once the day's files are opened RECORDS is no longer marked")))return;
  M->UI->Close(true);for(int32 I=0;I<4&&F.Day()<3;++I)F.TestAdvance(FMath::Fmod(8.2*3600.-FMath::Fmod(F.Clock(),86400.)+86400.,86400.)+60.);
  if(!Check(F.Day()>=3&&F.StationFileDue(),FString::Printf(TEXT("on day %d the memo on the valley in Norway is released and RECORDS is marked again"),F.Day())))return;
  Go(8,.5);return;
 }
 if(Stage==8){M->UI->Close(true);Show(TEXT("station-03"),0);if(!Check(F.BinderView->Tab==S47Binder::SecStation&&F.BinderView->Pick==S47Binder::FileKey(TEXT("station-03"))&&!F.TerminalOnGlass(),TEXT("the same files are in the records binder, on paper")))return;Go(9,.7);return;}
 if(Stage==9){Shot(TEXT("station-files-binder.png"));Go(10,.3);return;}
 if(Stage==10)
 {
  M->UI->Close(true);
  if(!Check(!F.Save()->HasField(TEXT("stationFiles")),TEXT("no file text is in the save")))return;
  // FIX10-10: a file released on an earlier day and never opened stays marked; what was opened is saved by key.
  if(!Check(F.StationFileDue(),TEXT("the files released on day 2 and never opened still mark RECORDS on a later day")))return;
  const TArray<FString> Seen=S47J::Strs(F.Save(),TEXT("stationFilesSeen"));
  if(!Check(Seen.Contains(TEXT("station-03"))&&Seen.Contains(TEXT("station-01"))&&!Seen.Contains(TEXT("station-07")),TEXT("the files opened are in the save by key, the unopened are not")))return;
  for(int32 I=0;I<S47StationFiles::Count();++I)if(S47StationFiles::Get(I).Day<=F.Day())Show(S47StationFiles::Get(I).Key,0);
  M->UI->Close(true);
  if(!Check(!F.StationFileDue(),TEXT("once every released file is opened RECORDS is no longer marked")))return;
  Finish(true,TEXT("Station files: nine of SARO's own papers under OPS / RECORDS and in the binder, released by day, paged, within the limits."));return;
 }
}

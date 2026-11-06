#include "S47MenuPointerTest.h"
#include "S47Main.h"
#include "S47ArrayWear.h"
#include "S47CalicheAccess.h"
#include "S47TownLayer.h"
#include "S47Herd.h"
#include "S47Taken.h"
#include "S47FieldRadio.h"
#include "S47FreeformWorld.h"
#include "S47Story.h"
#include "S47TripoCreature.h"
#include "S47Freeform.h"
#include "S47Album.h"
#include "S47Flow.h"
#include "S47Night.h"
#include "S47Panels.h"
#include "S47Kit.h"
#include "S47Glimpses.h"
#include "Components/PoseableMeshComponent.h"
#include "UObject/UObjectIterator.h"
#include "S47FarmHouse.h"
#include "S47FarmBuildings.h"
#include "S47FarmWorld.h"
#include "S47RoadShape.h"
#include "S47World.h"
#include "S47Inter.h"
#include "S47FreeformDirector.h"
#include "S47FarmSite.h"
#include "S47FarmNight.h"
#include "S47Three.h"
#include "S47Web.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "S47FieldCamera.h"
#include "S47RoomDecor.h"
#include "S47Walker.h"
#include "S47UI.h"
#include "S47Json.h"
#include "ImageUtils.h"
#include "Misc/Base64.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/Children.h"
#include "Layout/WidgetPath.h"
#include "Misc/CommandLine.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
 TSharedPtr<SWidget> FindText(const TSharedRef<SWidget>& W,const FString& Text)
 {
  if(!W->GetVisibility().IsVisible()) return nullptr;
  if(W->GetType()==FName(TEXT("STextBlock")) && StaticCastSharedRef<STextBlock>(W)->GetText().ToString()==Text)
  {
   const FVector2D Size=W->GetCachedGeometry().GetLocalSize();
   if(Size.X>1 && Size.Y>1) return W;
  }
  if(FChildren* Children=W->GetChildren())
   for(int32 I=0;I<Children->Num();++I) if(auto Found=FindText(Children->GetChildAt(I),Text)) return Found;
  return nullptr;
 }

 bool ClickText(const FString& Text)
 {
  if(!GEngine || !GEngine->GameViewport || !FSlateApplication::IsInitialized()) return false;
  const auto Window=GEngine->GameViewport->GetWindow();
  if(!Window) return false;
  const auto Widget=FindText(Window.ToSharedRef(),Text);
  if(!Widget) return false;
  const FGeometry& G=Widget->GetCachedGeometry();
  const FVector2D At=G.LocalToAbsolute(G.GetLocalSize()*.5f);
  auto& App=FSlateApplication::Get();
  const FWidgetPath Hit=App.LocateWindowUnderMouse(At,App.GetInteractiveTopLevelWindows());
  UE_LOG(LogTemp,Display,TEXT("S47: menu pointer hit '%s': %s"),*Text,Hit.IsValid()?*Hit.GetLastWidget()->GetTypeAsString():TEXT("no widget"));
  const TSet<FKey> None,Left={EKeys::LeftMouseButton};
  App.ProcessMouseMoveEvent(FPointerEvent(0,At,At-FVector2D(1,0),None,EKeys::Invalid,0.f,App.GetModifierKeys()));
  App.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(0,At,At,Left,EKeys::LeftMouseButton,0.f,App.GetModifierKeys()));
  // Cross at least one frame like a physical click. Capture only coordinates and
  // label: keeping the found widget/path alive here could hide lifetime bugs.
  FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([At,Text](float)
  {
   if(!FSlateApplication::IsInitialized()) return false;
   auto& ReleaseApp=FSlateApplication::Get();
   const TSet<FKey> Released;
   ReleaseApp.ProcessMouseButtonUpEvent(FPointerEvent(0,At,At,Released,EKeys::LeftMouseButton,0.f,ReleaseApp.GetModifierKeys()));
   UE_LOG(LogTemp,Display,TEXT("S47: menu pointer clicked '%s' at %.1f %.1f (synthetic Slate pointer, down/up across frames, no direct callback)"),*Text,At.X,At.Y);
   return false;
  }),.12f);
  return true;
 }

 struct FRun
 {
  TWeakObjectPtr<US47Main> Main;
  int32 Stage=0;
  double Elapsed=0,StageAt=0;
  bool bLethal=false,bQuit=false,bFullCases=false,bAlbum=false;
  int32 LeftBefore=0,SkyBefore=0,TownAt=0;
  FString TownPlace;
  double VisitClock=0,VisitFrom=0;double ChainFrom=0,ChainAt=0,ChainAge=0;bool ChainPaused=false;FString ChainEvent;double CowFrom=0;FVector CowA=FVector::ZeroVector,CowB=FVector::ZeroVector;int32 CowCalls=0;FString TakenPlace;bool TakenRoom=false,TakenEscape=false;int32 TakenKind=0;FVector TakenPos=FVector::ZeroVector;double TakenAt=0;bool TakenShot[5]={false,false,false,false,false};FVector FieldFrom=FVector::ZeroVector;int32 FieldShot=0;bool BlockTest=false;bool ShotPending=false;TArray<FVector> WalkTo;int32 WalkAt=0;double WalkFrom=0;
  // From the yard, at the farm: the lights' clear line of sight, measured on 6 November.
  void FaceFarm(US47Main* M)
  {
   const FVector Target(84,20,492);const FVector2D At(0,30);const FVector D=Target-FVector(At.X,1.7,At.Y);
   M->Player->Place(At.X,At.Y,FMath::Atan2(-D.X,-D.Z));M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
  }
  // Takes a real photograph and counts bright pixels in the middle of it, where the farm is.
  int32 Exposed(US47Main* M)
  {
   M->FieldCamera->Raise(true);M->FieldCamera->Sync();const auto Photo=M->Night->Album->Expose();M->FieldCamera->Raise(false);M->UI->Viewfinder(false);
   FString Base64;if(!Photo.Url.Split(TEXT(","),nullptr,&Base64))return -1;
   TArray<uint8> Bytes;if(!FBase64::Decode(Base64,Bytes))return -1;
   auto& Images=FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
   const EImageFormat Format=Images.DetectImageFormat(Bytes.GetData(),Bytes.Num());if(Format==EImageFormat::Invalid)return -1;
   const auto Image=Images.CreateImageWrapper(Format);if(!Image.IsValid()||!Image->SetCompressed(Bytes.GetData(),Bytes.Num()))return -1;
   TArray<uint8> Raw;if(!Image->GetRaw(ERGBFormat::RGBA,8,Raw))return -1;
   const int32 W=Image->GetWidth(),H=Image->GetHeight();int32 Count=0;
   for(int32 Y=H*2/5;Y<H*3/5;++Y)for(int32 X=W*3/10;X<W*7/10;++X){const uint8* P=&Raw[(Y*W+X)*4];if(P[0]>=170&&P[1]>=170&&P[2]>=150)++Count;}
   return Count;
  }
  void Shot(const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("AlbumTest")/Name,true,false);}
  void Press(const FKey& Key)
  {
   auto& App=FSlateApplication::Get();
   const FKeyEvent E(Key,App.GetModifierKeys(),0,false,0,0);
   App.ProcessKeyDownEvent(E);App.ProcessKeyUpEvent(E);
  }
  bool Sees(const TCHAR* Text) const
  {
   const auto Window=GEngine&&GEngine->GameViewport?GEngine->GameViewport->GetWindow():nullptr;
   return Window&&FindText(Window.ToSharedRef(),Text).IsValid();
  }
  // Two prints: an ordinary one, and one the game saw something in, which has to stay.
  void GiveAlbum(US47Main* M)
  {
   TArray<FColor> Pixels;Pixels.SetNum(320*240);
   for(int32 I=0;I<Pixels.Num();++I)Pixels[I]=FColor(uint8(40+(I%320)/2),uint8(60+(I/320)/2),90);
   TArray64<uint8> Png;FImageUtils::PNGCompressImageArray(320,240,Pixels,Png);
   const FString Url=TEXT("data:image/png;base64,")+FBase64::Encode(Png.GetData(),int32(Png.Num()));
   TArray<TSharedPtr<FJsonValue>> Shots;
   for(int32 I=0;I<2;++I)
   {
    const auto O=S47J::New();
    S47J::Set(O,TEXT("id"),FString::Printf(TEXT("albumtest%d@10800"),I));S47J::Set(O,TEXT("url"),Url);S47J::Set(O,TEXT("clock"),10800.);
    S47J::Set(O,TEXT("place"),TEXT("Service yard"));S47J::Set(O,TEXT("text"),I?TEXT("The print the game saw something in."):TEXT("An ordinary print."));S47J::Set(O,TEXT("labStage"),3);
    if(I){const auto T=S47J::New();S47J::Set(T,TEXT("anomaly:albumtest"),TArray<double>{.5,.5});S47J::Set(O,TEXT("targets"),T);}
    Shots.Add(MakeShared<FJsonValueObject>(O));
   }
   const auto V=S47J::New();V->SetArrayField(TEXT("shots"),Shots);
   M->Night->Album->Restore(V);
   LeftBefore=M->Night->Album->Left();
  }
  bool Fail(const TCHAR* Why)
  {
   UE_LOG(LogTemp,Error,TEXT("S47: menu pointer FAIL: %s (stage %d)"),Why,Stage);
   FPlatformMisc::RequestExitWithStatus(false,1);
   return false;
  }
  bool Step(float Dt)
  {
   Elapsed+=Dt;
   if(!Main.IsValid()) return Fail(TEXT("main disappeared"));
   auto* M=Main.Get();
   if(Elapsed-StageAt>45) return Fail(TEXT("menu click did not reach its expected next screen"));
   if(Stage!=77&&Stage!=212&&Elapsed-StageAt<1) return true;
   const FString Label=bQuit?TEXT("QUIT"):TEXT("NEW FREEFORM");
   if(Stage==0)
   {
    FSlateApplication::Get().ProcessApplicationActivationEvent(true);
    if(ClickText(TEXT("PRESS ANY KEY")) || ClickText(Label)){Stage=1;StageAt=Elapsed;} // Ordinary title gate, then the visible menu.
    return true;
   }
   if(Stage==1)
   {
    if(!M->ModalOpen()) ClickText(Label); // Second click selects it after the gate, if necessary.
    Stage=bQuit?4:2;StageAt=Elapsed;
    return true;
   }
   if(Stage==2)
   {
    if(ClickText(bLethal?TEXT("Death / reload last save"):TEXT("Local recovery / recommended")))
    {Stage=bFullCases?5:3;StageAt=Elapsed;}
    return true;
   }
   if(Stage==5)
   {
    if(M->Mode==US47Main::EMode::Play) return Fail(TEXT("full-cases fixture unexpectedly bypassed replacement choice"));
    if(ClickText(TEXT("CASE 1"))){Stage=6;StageAt=Elapsed;}
    return true;
   }
   if(Stage==6)
   {
    if(ClickText(TEXT("START NEW FREEFORM"))){Stage=3;StageAt=Elapsed;}
    return true;
   }
   if(Stage==3)
   {
    if(M->Mode!=US47Main::EMode::Play) return true;
    if(!M->IsFreeform() || !M->Freeform || !M->Freeform->Active()) return Fail(TEXT("menu started a non-Freeform session"));
    if(M->Freeform->Lethal()!=bLethal || M->Freeform->Day()!=1 || M->Freeform->Jobs().Num()==0 || M->ModalOpen())
     return Fail(TEXT("Freeform consequences, daily work or closed-menu state is incorrect"));
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerEyes")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the eye test needs the night"));
     M->Player->Place(3.18,3.7,0);M->Player->Pitch=-.2;
     Stage=99;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerVisit")))
    {
     M->Flow->bDevRun=true;VisitFrom=Elapsed;
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the visit test needs the night"));
     if(FParse::Param(FCommandLine::Get(),TEXT("S47VisitTown")))
     {
      M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));Stage=139;VisitFrom=Elapsed;StageAt=Elapsed;return true;
     }
     M->Freeform->FarmSite->ArmFarm(M->Freeform->Clock());
     const FVector C=M->Freeform->FarmSite->Centre();M->Player->Place(C.X+8,C.Z+8,0);
     VisitClock=M->Freeform->Clock();M->Freeform->FarmSite->TestVisit();Stage=140;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerTaken")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the taken test needs the night"));
     FParse::Value(FCommandLine::Get(),TEXT("S47TakenKind="),TakenKind);TakenEscape=FParse::Param(FCommandLine::Get(),TEXT("S47TakenEscape"));
     FParse::Value(FCommandLine::Get(),TEXT("S47TakenPlace="),TakenPlace);
     if(!TakenPlace.IsEmpty())
     {
      M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));VisitFrom=Elapsed;Stage=149;StageAt=Elapsed;return true;
     }
     M->Freeform->FarmSite->ArmFarm(M->Freeform->Clock());
     // Inside the old house for the light through its windows, out on the pasture for the crab.
     const FVector C=M->Freeform->FarmSite->Centre();const FVector In=S47FarmHouse::Point(2.5,2.4)+FVector(-24,0,300);
     if(TakenKind==1)M->Player->Place(C.X+8,C.Z+8,0);else M->Player->Place(In.X,In.Z,0);
     M->Player->Yaw=0;M->Player->Pitch=0;
     VisitClock=M->Freeform->Clock();VisitFrom=Elapsed;TakenPos=FVector(M->Player->Pos.X,0,M->Player->Pos.Z);
     M->Freeform->FarmSite->TestVisit(TakenKind);Stage=150;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerCows")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     M->Player->Place(0,-1.2,PI);M->Player->Pitch=-.05;CowFrom=Elapsed;Stage=160;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerChain")))
    {
     M->Flow->bDevRun=true;
     if(!M->Freeform->TestAdvance(37.*3600.))return Fail(TEXT("the chain needs night two"));
     M->Freeform->TestEndFirstShift();M->Freeform->Director->TestLightsTonight();ChainFrom=Elapsed;Stage=170;StageAt=Elapsed;
     UE_LOG(LogTemp,Display,TEXT("S47: chain: night two, day %d, clock %.0f, area %s"),M->Freeform->Day(),M->Freeform->Clock(),*M->Web->Area());
     return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerPanels")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();M->Freeform->TestEndFirstShift();
     M->Freeform->Field->AddItem(TEXT("relay"),3);M->Freeform->Field->AddItem(TEXT("fuse"),2);
     Stage=180;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerStare")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the stare needs the night"));
     M->Freeform->FarmSite->SetCalm(1e12);M->Freeform->FarmSite->ArmFarm(M->Freeform->Clock());
     const FVector C=M->Freeform->FarmSite->Centre();M->Player->Place(C.X+6,C.Z+18,0);
     CowFrom=Elapsed;Stage=190;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerField")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     M->Freeform->Field->TestService(TEXT("S-08"),3);Stage=120;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerRoad")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the road test needs the night"));
     M->World->Enter(TEXT("road"));Stage=80;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerBuildings")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();Stage=200;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerHouse")))
    {
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();Stage=70;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerTown")))
    {
     M->Flow->bDevRun=true;
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the town test needs the night"));
     M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));Stage=60;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerNature")))
    {
     // The rules of nature in Freeform: day shots at eleven from six places, and the pasture at night.
     M->Flow->bDevRun=true;if(M->Freeform->Director)M->Freeform->Director->Stop();
     const double Today=FMath::Fmod(M->Freeform->Clock(),86400.);
     if(!M->Freeform->TestAdvance(FMath::Fmod(11*3600.-Today+86400.,86400.)))return Fail(TEXT("the nature test needs the day"));
     Stage=200;StageAt=Elapsed;return true;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerFarm")))
    {
     M->Flow->bDevRun=true;
     if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the farm test needs the night"));
     FaceFarm(M);Stage=49;StageAt=Elapsed;return true;
    }
    if(bAlbum){GiveAlbum(M);M->UI->NotebookAt(TEXT("photos"));M->Flow->OpenNotebook();Stage=10;StageAt=Elapsed;return true;}
    UE_LOG(LogTemp,Display,TEXT("S47: menu pointer PASS: title -> NEW FREEFORM -> %s -> active day 1 with %d jobs"),bLethal?TEXT("death"):TEXT("local recovery"),M->Freeform->Jobs().Num());
    FPlatformMisc::RequestExitWithStatus(false,0);
    return false;
   }
   // The new farm buildings, in the saro frame: a look at each from the yard by day and by night and one inside the
   // barn, then in through its door on foot and on until the back wall stops the walk.
   if(Stage==205)
   {
    if(!M->Freeform->TestAdvance(13.*3600.))return Fail(TEXT("the buildings test needs the night"));
    Stage=206;StageAt=Elapsed;return true;
   }
   if(Stage>=200&&Stage<=210)
   {
    if(!S47FarmWorld::BuildingsShown(M))return Fail(TEXT("the four farm buildings must stand and show; was the import run?"));
    struct FView{const TCHAR* Name;double X,Z,LookX,LookHeight,LookZ;};
    const FVector Aisle=S47FarmBuildings::BarnPoint(1.3,-4),BackOfAisle=S47FarmBuildings::BarnPoint(1.3,4.6);
    const FView Views[]={{TEXT("barn"),137.5,186,136,3,172},{TEXT("house"),125,191,110,1.8,190},{TEXT("shedA"),117,180,116.5,1.5,170},{TEXT("shedB"),90,213,84.5,1.5,206.5},
     {TEXT("barn-inside"),Aisle.X,Aisle.Z,BackOfAisle.X,1.8,BackOfAisle.Z}};
    const bool bNight=Stage>=206;const int32 I=Stage-(bNight?206:200);
    if(ShotPending)
    {
     if(I==4&&!FMath::IsNearlyEqual(M->Player->FloorY,S47FarmBuildings::BarnPoint(0,0).Y,.01))return Fail(TEXT("the barn's floor must be level"));
     Shot(*FString::Printf(TEXT("buildings-%s-%s.png"),bNight?TEXT("night"):TEXT("day"),Views[I].Name));ShotPending=false;Stage=Stage==210?211:Stage+1;StageAt=Elapsed;return true;
    }
    const FVector O(-24,0,300);const FVector2D D(Views[I].LookX-Views[I].X,Views[I].LookZ-Views[I].Z);
    M->Player->Place(Views[I].X+O.X,Views[I].Z+O.Z,FMath::Atan2(-D.X,-D.Y));M->Player->Pitch=FMath::Atan2(Views[I].LookHeight-1.62,D.Size());ShotPending=true;
    StageAt=Elapsed;return true;
   }
   if(Stage==211)
   {
    if(S47FarmWorld::WindowGlowsAtHouse(M)!=3)return Fail(TEXT("the lit window's three glows must follow the house"));
    const FVector O(-24,0,300);
    WalkTo={S47FarmBuildings::BarnPoint(1.3,-8)+O,S47FarmBuildings::BarnPoint(1.3,-2)+O,S47FarmBuildings::BarnPoint(1.3,7)+O};WalkAt=1;
    M->Player->Place(WalkTo[0].X,WalkTo[0].Z,0);Stage=212;StageAt=Elapsed;WalkFrom=Elapsed;return true;
   }
   if(Stage==212)
   {
    StageAt=Elapsed;
    const FVector V=WalkTo[WalkAt]-M->Player->Pos;const double Left=FVector2D(V.X,V.Z).Size();
    if(WalkAt==1)
    {
     if(Elapsed-WalkFrom>20)return Fail(TEXT("the barn's door must let the player in"));
     if(Left<.3)
     {
      if(!FMath::IsNearlyEqual(M->Player->FloorY,S47FarmBuildings::BarnPoint(0,0).Y,.02))return Fail(TEXT("the barn's floor must carry the player"));
      WalkAt=2;WalkFrom=Elapsed;return true;
     }
    }
    else if(Elapsed-WalkFrom>8)
    {
     // Its road-local z falls toward the back wall. Stopped close to it, and not through it.
     M->Player->TestMove.Reset();
     const double Reached=M->Player->Pos.Z-300;const double WallZ=S47FarmBuildings::BarnPoint(1.3,5.4).Z,NearZ=S47FarmBuildings::BarnPoint(1.3,4.6).Z;
     if(Reached<WallZ)return Fail(TEXT("the barn's back wall must stop the player"));
     if(Reached>NearZ)return Fail(TEXT("the barn's aisle must run clear to the back wall"));
     Shot(TEXT("buildings-night-barn-wall.png"));
     UE_LOG(LogTemp,Display,TEXT("S47: buildings PASS: four buildings shown day and night, glow at the house, in through the barn's door, level floor, stopped %.2f m from the back wall"),Reached-WallZ);
     FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    M->Player->Yaw=FMath::Atan2(-V.X,-V.Z);M->Player->Pitch=-.08;M->Player->TestMove=FVector2D(0,-FMath::Min(1.,Left/.4));
    return true;
   }
   // The abandoned house, in the saro frame the farm lies in: a look from each named spot, then the real walk.
   if(Stage>=70&&Stage<=75)
   {
    struct FView{const TCHAR* Name;double X,Y,LookX,LookY;};
    static const FView Views[]={{TEXT("outside"),6,16,6,4},{TEXT("hall"),6,8.3,6,0},{TEXT("bedroom"),4.8,2.25,1,2.25},{TEXT("living"),7.2,6.55,11,6.55},{TEXT("kitchen"),4.8,6.55,1,6.55},{TEXT("back"),8.4,2.25,8.4,1}};
    const int32 I=Stage-70;
    if(ShotPending){Shot(*FString::Printf(TEXT("house-%s.png"),Views[I].Name));ShotPending=false;Stage=I==5?76:Stage+1;StageAt=Elapsed;return true;}
    const FVector O(-24,0,300);const FVector At=S47FarmHouse::Point(Views[I].X,Views[I].Y)+O,Look=S47FarmHouse::Point(Views[I].LookX,Views[I].LookY)+O;const FVector D=Look-At;
    M->Player->Place(At.X,At.Z,FMath::Atan2(-D.X,-D.Z));M->Player->Pitch=0;ShotPending=true;
    if(I>0&&!FMath::IsNearlyEqual(M->Player->FloorY,S47FarmHouse::PadY(),.01))return Fail(TEXT("the floor inside the house must be level"));
    StageAt=Elapsed;return true;
   }
   if(Stage==76)
   {
    // First against the shut front door: it has to stop the player, then opening it has to clear the way.
    const FVector O(-24,0,300);auto& H=*M->Freeform->HouseDoors;
    if(H.Num()!=7||H.IsOpen(H.IndexOf(TEXT("south0"))))return Fail(TEXT("the house has seven doors and its front door starts shut"));
    WalkTo={S47FarmHouse::Point(6,9.6)+O,S47FarmHouse::Point(6,4.5)+O};WalkAt=0;BlockTest=true;
    M->Player->Place(WalkTo[0].X,WalkTo[0].Z-2,0);Stage=77;StageAt=Elapsed;WalkFrom=Elapsed;return true;
   }
   if(Stage==77)
   {
    StageAt=Elapsed;
    if(BlockTest&&Elapsed-WalkFrom>7)
    {
     const FVector Door=S47FarmHouse::Point(6,8.85)+FVector(-24,0,300);
     // The hall is farther into +Z than the door: past it means the shut door failed.
     if(M->Player->Pos.Z>Door.Z+.05)return Fail(TEXT("a shut door must stop the player"));
     M->Player->TestMove.Reset();auto& H=*M->Freeform->HouseDoors;
     const auto Saved=H.Save();
     if(!H.Use(H.IndexOf(TEXT("south0")))||!H.Use(H.IndexOf(TEXT("north0"))))return Fail(TEXT("the doors must open"));
     H.Begin(H.Save());if(!H.IsOpen(H.IndexOf(TEXT("south0"))))return Fail(TEXT("door state must survive a reload"));
     H.Begin(Saved);if(H.IsOpen(H.IndexOf(TEXT("south0"))))return Fail(TEXT("an older saved state must restore"));
     H.Use(H.IndexOf(TEXT("south0")));H.Use(H.IndexOf(TEXT("north0")));
     Shot(TEXT("house-door-open.png"));
     const FVector O(-24,0,300);WalkTo={S47FarmHouse::Point(6,4.5)+O,S47FarmHouse::Point(6,-1)+O};WalkAt=0;BlockTest=false;WalkFrom=Elapsed;return true;
    }
    if(Elapsed-WalkFrom>30)return Fail(TEXT("the hall must be walkable from the front door to the back door"));
    if(BlockTest&&WalkAt>=WalkTo.Num())return Fail(TEXT("a shut door must stop the player"));
    if(WalkAt>=WalkTo.Num())
    {
     M->Player->TestMove.Reset();
     UE_LOG(LogTemp,Display,TEXT("S47: farmhouse PASS: level floor, shut door stopped the player, opened, walked through and out of the back in %.1f s"),Elapsed-WalkFrom);
     FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    const FVector V=WalkTo[WalkAt]-M->Player->Pos;const double Left=FVector2D(V.X,V.Z).Size();
    if(Left<.3){++WalkAt;return true;}
    M->Player->Yaw=FMath::Atan2(-V.X,-V.Z);M->Player->Pitch=-.08;M->Player->TestMove=FVector2D(0,-FMath::Min(1.,Left/.4));
    return true;
   }
   // The way out at night, from the road side and from the farm, in the frame each is really seen in.
   if(Stage>=80&&Stage<87)
   {
    struct FV{const TCHAR* Name;const TCHAR* Area;double X,Z,LX,LZ;double Lift;};
    const FVector Sign=FS47FarmNight::SignWorld();
    const FV V[]={{TEXT("turnoff-far"),TEXT("road"),8001,110,8007,196,6},{TEXT("turnoff-near"),TEXT("road"),8002,178,8007,196,3},{TEXT("sign"),TEXT("road"),Sign.X,Sign.Z-24,Sign.X,Sign.Z,3},{TEXT("road-in"),TEXT("saro"),-16,497,16,497,0},{TEXT("road-mid"),TEXT("saro"),30,501,70,497,0},{TEXT("road-end"),TEXT("saro"),100,503,60,497,0},{TEXT("trafo-entry"),TEXT("road"),8003,460,7997,520,4}};
    const int32 I=Stage-80;
    if(ShotPending){Shot(*FString::Printf(TEXT("road-%s.png"),V[I].Name));ShotPending=false;Stage=I==6?90:Stage+1;StageAt=Elapsed;return true;}
    if(M->Web->Area()!=V[I].Area){M->World->Enter(V[I].Area);StageAt=Elapsed;return true;}
    const FVector D(V[I].LX-V[I].X,V[I].Lift,V[I].LZ-V[I].Z);M->Player->Place(V[I].X,V[I].Z,FMath::Atan2(-D.X,-D.Z));M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());ShotPending=true;StageAt=Elapsed;
    return true;
   }
   if(Stage==90)
   {
    if(!M->Freeform->FarmNight->LampsOn())return Fail(TEXT("the lamps must be lit at night"));
    M->World->Enter(TEXT("saro"));const FVector C=FVector(84,0,492);M->Player->Place(C.X,C.Z,0);Stage=91;StageAt=Elapsed;WalkFrom=Elapsed;return true;
   }
   if(Stage==91)
   {
    StageAt=Elapsed;
    if(Elapsed-WalkFrom<45&&M->Freeform->FarmNight->FarmSounds()<2)return true;
    if(M->Freeform->FarmNight->FarmSounds()<2)return Fail(TEXT("the farm must make sounds of its own"));
    if(!M->Freeform->TestAdvance(12.*3600.))return Fail(TEXT("advancing to day"));
    Stage=92;WalkFrom=Elapsed;return true;
   }
   if(Stage==92)
   {
    StageAt=Elapsed;if(Elapsed-WalkFrom<1)return true;
    if(M->Freeform->FarmNight->LampsOn())return Fail(TEXT("the lamps must be off by day"));
    UE_LOG(LogTemp,Display,TEXT("S47: farm road PASS: lamps lit at night and off by day, %d farm sounds played"),M->Freeform->FarmNight->FarmSounds());
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   // The nature: the pasture, the roadside south of SARO, flat desert towards the mountains, the antenna field, the edge of Caliche and
   // STATION 01 by day, then the pasture at night. Each view waits for the plants to fill in before its picture.
   if(Stage>=200&&Stage<=206)
   {
    struct FN{const TCHAR* Name;const TCHAR* Area;double X,Z,LX,LZ,Lift;};
    const auto Saro=[](double X,double Z){return S47FarmWorld::RoadToArea(TEXT("saro"),FVector(X,0,Z));};
    const FVector Past0=Saro(70,240),Past1=Saro(76,251);
    const double Edge=S47RoadShape::TOWN().Z0;
    const FN V[]={{TEXT("pasture-day"),TEXT("saro"),Past0.X,Past0.Z,Past1.X,Past1.Z,0},
     {TEXT("roadside"),TEXT("road"),8010,40,8013,140,0},
     {TEXT("flat-desert"),TEXT("road"),8070,300,8300,300,12},
     {TEXT("antenna-field"),TEXT("saro"),-1,-30,-1,-110,0},
     {TEXT("caliche-edge"),TEXT("diner"),7995,Edge-50,7995,Edge+30,0},
     {TEXT("station01"),TEXT("station01"),-4,8014,-11,8003,0},
     {TEXT("pasture-night"),TEXT("saro"),Past0.X,Past0.Z,Past1.X,Past1.Z,0}};
    const int32 I=Stage-200;
    StageAt=Elapsed;
    if(ShotPending)
    {
     if(Elapsed-WalkFrom<4)return true;
     Shot(*FString::Printf(TEXT("nature-%s.png"),V[I].Name));ShotPending=false;
     if(I==5){if(!M->Freeform->TestAdvance(12*3600.))return Fail(TEXT("the nature test needs the night"));Stage=206;}
     else Stage=I==6?207:Stage+1;
     return true;
    }
    if(M->Web->Area()!=V[I].Area){M->World->Enter(V[I].Area);return true;}
    const FVector D(V[I].LX-V[I].X,V[I].Lift,V[I].LZ-V[I].Z);
    M->Player->Place(V[I].X,V[I].Z,FMath::Atan2(-D.X,-D.Z));M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
    ShotPending=true;WalkFrom=Elapsed;
    return true;
   }
   if(Stage==207)
   {
    UE_LOG(LogTemp,Display,TEXT("S47: nature shown"));
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   if(Stage==99)
   {
    if(Elapsed-StageAt<3)return true;
    if(M->Night->Glimpses->DevSpawn(1,FS47RoswellBrain::EMode::Stand)!=1)return Fail(TEXT("the eye test needs one body"));
    Stage=100;StageAt=Elapsed;return true;
   }
   // Right up to the face, then out in the open at night from a distance, the player's own height and no torch.
   if(Stage>=100&&Stage<=108)
   {
    UPoseableMeshComponent* Skin=nullptr;
    for(TObjectIterator<UPoseableMeshComponent> It;It;++It)if(It->GetWorld()==M->Web->GetWorld()&&It->IsVisible()&&It->GetName().Contains(TEXT("Roswell")))Skin=*It;
    if(!Skin)return Fail(TEXT("no visible Roswell body"));
    const FVector Head=S47Three::ToThree(Skin->GetBoneLocationByName(TEXT("head"),EBoneSpaces::WorldSpace));
    const FVector Face=(S47Three::ToThree(Skin->GetBoneLocationByName(TEXT("head"),EBoneSpaces::WorldSpace)+Skin->GetRightVector()*100)-Head).GetSafeNormal();
    auto Look=[&](double Away)
    {
     const FVector At=Head+Face*Away;M->Player->Place(At.X,At.Z,0);
     M->Player->Eye=Away<1?Head.Y-M->Player->FloorY+.08:1.62;
     const FVector Eye(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z),D=Head+FVector(0,Away<1?.08:Away*.03,0)-Eye; // From afar, aim a little over the head so the crosshair does not sit on the eyes.
     M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
    };
    switch(Stage)
    {
     case 100:Look(.5);break;
     case 101:Shot(TEXT("eyes-close.png"));break;
     case 102:M->Night->Glimpses->DevClear();M->Player->Eye=1.62;M->Player->Place(0,30,0);M->Player->Pitch=-.05;Stage=109;StageAt=Elapsed;return true;
     case 103:Look(10);break;
     case 104:Shot(TEXT("eyes-10m.png"));break;
     case 105:Look(25);break;
     case 106:Shot(TEXT("eyes-25m.png"));break;
     case 107:
      if(FS47TripoCreature::ShownEyeGlints()<2)return Fail(TEXT("the eyes must show at 25 m at night"));
      Look(60);Stage=110;StageAt=Elapsed;return true;
    }
    ++Stage;StageAt=Elapsed;return true;
   }
   if(Stage==149)
   {
    StageAt=Elapsed;
    if(Elapsed-VisitFrom<2.5)return true;
    FVector In;
    if(!S47CalicheAccess::RoomMiddle(TakenPlace,In))return Fail(TEXT("no such room"));
    const FVector P=M->Freeform->Director->LightPlacePoint(TakenPlace);
    M->Freeform->FarmSite->Arm(M->Freeform->Clock(),TakenPlace,TEXT("the place"),TEXT("diner"),P.IsZero()?In:P);
    M->Player->Place(In.X,In.Z,0);M->Player->Pitch=0;
    UE_LOG(LogTemp,Display,TEXT("S47: taken test in %s at %s"),*TakenPlace,*In.ToCompactString());
    VisitClock=M->Freeform->Clock();VisitFrom=Elapsed;TakenPos=FVector(M->Player->Pos.X,0,M->Player->Pos.Z);
    M->Freeform->FarmSite->TestVisit(TakenKind);Stage=150;return true;
   }
   // Left where they stand: each kind must take them, and leave them where and when its terms say.
   if(Stage>=150&&Stage<=154)
   {
    StageAt=Elapsed;
    auto& Tk=*M->Freeform->Taken;auto& Site=*M->Freeform->FarmSite;
    // Walls can hold, in a town room: staying put and unseen is one way for it to pass.
    if(!TakenPlace.IsEmpty()&&Site.VisitStage()==4&&!Tk.Busy()&&Tk.LastNote().IsEmpty())
    {UE_LOG(LogTemp,Display,TEXT("S47: taken PASS in %s: the walls held"),*TakenPlace);FPlatformMisc::RequestExitWithStatus(false,0);return false;}
    if(Elapsed-VisitFrom>170)return Fail(TEXT("the taker must take a player who stays"));
    // The way out: stand out of the light, and it passes with nothing lost.
    if(TakenEscape)
    {
     if(Tk.Stage()==FS47Taken::EStage::Flood&&!TakenShot[1])
     {
      TakenShot[1]=true;const FVector Aside=S47FarmHouse::Point(3,6.5)+FVector(-24,0,300);M->Player->Place(Aside.X,Aside.Z,0);
      UE_LOG(LogTemp,Display,TEXT("S47: taken test: out of the light, into the next room"));return true;
     }
     if(TakenShot[1]&&!Tk.Busy()&&Site.VisitStage()!=4)
     {
      // The body is still about; this test is the light. Done with the visit.
      if(M->Freeform->Clock()>VisitClock+10*60)return Fail(TEXT("getting out of the light must cost no time"));
      UE_LOG(LogTemp,Display,TEXT("S47: taken PASS: out of the light, %.1f minutes passed"),(M->Freeform->Clock()-VisitClock)/60.);
      FPlatformMisc::RequestExitWithStatus(false,0);return false;
     }
     return true;
    }
    {static double NextLog=0;if(Elapsed>=NextLog){NextLog=Elapsed+5;UE_LOG(LogTemp,Display,TEXT("S47: taken watch, stage %d, visit %d, in light %.1f, body %.1f m, indoors %d"),int32(Tk.Stage()),Site.VisitStage(),Tk.LightOn(),Site.VisitBodyDistance(),Tk.Indoors());}}
    if(Stage==150)
    {
     if(Tk.Stage()==FS47Taken::EStage::Dread&&!TakenShot[0]&&Elapsed-VisitFrom>3){TakenShot[0]=true;Shot(TEXT("taken-dread.png"));return true;}
     if(Tk.Stage()==FS47Taken::EStage::Flood&&!TakenShot[1])
     {
      TakenShot[1]=true;TakenAt=Elapsed;
     }
     if(TakenShot[1]&&!TakenShot[2]&&Tk.Stage()==FS47Taken::EStage::Flood&&Elapsed-TakenAt>1.2)
     {
      // Look at where it comes in, then round at the room it lights.
      const FVector D=Tk.LightFrom()-FVector(M->Player->Pos.X,M->Player->FloorY+1.6,M->Player->Pos.Z);
      M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
      TakenShot[2]=true;TakenAt=Elapsed;return true;
     }
     if(TakenShot[2]&&!TakenShot[3]&&Elapsed-TakenAt>.6){TakenShot[3]=true;Shot(TEXT("taken-flood.png"));TakenAt=Elapsed;M->Player->Yaw+=PI;M->Player->Pitch=-.25;return true;}
     if(TakenKind==1&&!TakenShot[3]&&Site.VisitBodyDistance()>0&&Site.VisitBodyDistance()<7)
     {
      const FVector Fe(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);(void)Fe;
      TakenShot[3]=true;Shot(TEXT("taken-near.png"));return true;
     }
     if(TakenShot[3]&&!TakenRoom&&Elapsed-TakenAt>.6&&Tk.Stage()==FS47Taken::EStage::Flood){TakenRoom=true;Shot(TEXT("taken-room.png"));return true;}
     if(Tk.Stage()==FS47Taken::EStage::Lift){TakenAt=Elapsed;Stage=151;return true;}
     if(Tk.Stage()==FS47Taken::EStage::Away){Stage=152;return true;}
     return true;
    }
    if(Stage==151)
    {
     if(Elapsed-TakenAt>3.5&&!TakenShot[4]){TakenShot[4]=true;Shot(TEXT("taken-lift.png"));}
     if(Tk.Stage()==FS47Taken::EStage::Away)Stage=152;
     return true;
    }
    if(Stage==152)
    {
     if(Tk.Busy())return true;
     const FS47TakenTerms& T=FS47Taken::Terms(TakenKind==1?ES47Taker::Crab:TakenKind==2?ES47Taker::Tall:ES47Taker::Roswell);
     const double Gone=(M->Freeform->Clock()-VisitClock)/60.;
     const double Moved=FVector(M->Player->Pos.X-TakenPos.X,0,M->Player->Pos.Z-TakenPos.Z).Size();
     UE_LOG(LogTemp,Display,TEXT("S47: taken: %.0f minutes gone, moved %.0f m, area %s. %s"),Gone,Moved,*M->Web->Area(),*Tk.LastNote());
     if(Gone<T.MinMinutes-3||Gone>T.MaxMinutes+12)return Fail(TEXT("the time gone must be within this taker's terms"));
     if(T.bHome&&FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(9.2,1.2))>2)return Fail(TEXT("the tall one must leave them at SARO"));
     if(T.WakeMax>0&&Moved<T.WakeMin-1)return Fail(TEXT("the crab must leave them away from the place"));
     if(!T.bHome&&T.WakeMax==0&&Moved>4)return Fail(TEXT("the small ones must leave them where they stood"));
     if(M->Player->CameraOverride&&Tk.Busy())return Fail(TEXT("the camera must be given back"));
     TakenAt=Elapsed;Stage=153;return true;
    }
    if(Stage==153)
    {
     if(Elapsed-TakenAt<1.5)return true;
     const double C0=M->Freeform->Clock();(void)C0;
     Shot(TEXT("taken-wake.png"));TakenAt=Elapsed;Stage=154;return true;
    }
    if(Elapsed-TakenAt<1)return true;
    if(Site.VisitStage()!=4)return Fail(TEXT("the visit must be over after waking"));
    UE_LOG(LogTemp,Display,TEXT("S47: taken PASS kind %d"),TakenKind);
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   // Air in the rooms, and cattle that live: they wander, they call, and something near sends them off.
   if(Stage>=160&&Stage<=166)
   {
    StageAt=Elapsed;
    auto* H=M->TownLayer&&!M->TownLayer->Herds.IsEmpty()?M->TownLayer->Herds[0].Get():nullptr;
    if(!H)return Fail(TEXT("no herd"));
    if(Stage==160){if(Elapsed-CowFrom<3)return true;Shot(TEXT("haze-control.png"));CowFrom=Elapsed;Stage=161;return true;}
    if(Stage==161)
    {
     if(Elapsed-CowFrom<1)return true;
     // Out in the pasture among them, looking across the herd.
     FVector Herd;H->CowAt(4,Herd);M->Player->Place(Herd.X+3,Herd.Z+24,0);M->Player->Pitch=-.04;
     H->CowAt(0,CowA);CowFrom=Elapsed;Stage=162;return true;
    }
    if(Stage==162)
    {
     if(Elapsed-CowFrom<25)return true;
     FVector Now;H->CowAt(0,Now);H->CowAt(1,CowB);
     UE_LOG(LogTemp,Display,TEXT("S47: cows: first moved %.2f m in 25 s, %d calls"),FVector::Dist(CowA,Now),M->Freeform->FarmNight->MooCount());
     Shot(TEXT("cows-calm.png"));CowA=Now;
     FVector Mid;H->CowAt(4,Mid);M->Freeform->FarmNight->TestAlarmAt=FVector(Mid.X,0,Mid.Z);CowCalls=M->Freeform->FarmNight->MooCount();CowFrom=Elapsed;Stage=163;return true;
    }
    if(Stage==163)
    {
     if(Elapsed-CowFrom<5)return true;
     Shot(TEXT("cows-panic.png"));Stage=164;return true;
    }
    if(Elapsed-CowFrom<9)return true;
    FVector Now;H->CowAt(1,Now);const double Ran=FVector::Dist(Now,CowB);const int32 Calls=M->Freeform->FarmNight->MooCount()-CowCalls;
    UE_LOG(LogTemp,Display,TEXT("S47: cows: ran %.1f m, %d frightened calls"),Ran,Calls);
    if(Ran<8)return Fail(TEXT("frightened cattle must run"));
    if(Calls<2)return Fail(TEXT("frightened cattle must call"));
    UE_LOG(LogTemp,Display,TEXT("S47: cows PASS"));
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   // The whole way with nothing forced: the scheduler's own draw, the lights, going there, the marks, standing about,
   // running into a building. Only the waits and the journey are shortened.
   if(Stage>=170&&Stage<=176)
   {
    StageAt=Elapsed;
    auto& D=*M->Freeform->Director;auto& Site=*M->Freeform->FarmSite;
    if(Elapsed-ChainFrom>150)return Fail(TEXT("the chain stopped"));
    if(Stage==170)
    {
     if(D.ActiveEvent().IsEmpty())return true;
     if(D.ActiveEvent()!=TEXT("farm_lights")&&D.ActiveEvent()!=TEXT("town_lights"))return true;
     ChainEvent=D.ActiveEvent();
     UE_LOG(LogTemp,Display,TEXT("S47: chain: the night's draw started %s; objective: %s"),*ChainEvent,*M->Freeform->Objective());
     if(!M->Freeform->Objective().Contains(TEXT("RADIO TIP")))return Fail(TEXT("the tip must stand on the objective line"));
     // Go there as the player would end up: on foot at the place, in its own area.
     if(ChainEvent==TEXT("town_lights")){M->World->PlaceTruck(TEXT("diner"));M->World->Enter(TEXT("diner"));}
     ChainAt=Elapsed;Stage=171;return true;
    }
    if(Stage==171)
    {
     if(Elapsed-ChainAt<1.5)return true;
     const FVector A=D.EventAnchor();M->Player->Place(A.X+30,A.Z,0);
     // In town the lights rise when someone is near, however long they have been there.
     ChainAt=Elapsed;Stage=172;return true;
    }
    if(Stage==172)
    {
     if(!D.ActiveEvent().IsEmpty()){const FVector A=D.EventAnchor();M->Player->Place(A.X+10,A.Z,0);return true;}
     if(!Site.Armed())return Fail(TEXT("the lights must leave marks behind"));
     UE_LOG(LogTemp,Display,TEXT("S47: chain: lights gone, marks at %s, %s"),*Site.PlaceId(),*Site.Centre().ToCompactString());
     Site.TestReadyVisit(); // the 60 percent, taken as given
     const FVector C=Site.Centre();M->Player->Place(C.X+6,C.Z+6,0);ChainAt=Elapsed;Stage=173;return true;
    }
    if(Stage==173)
    {
     // Twelve seconds standing about, on its own.
     if(Site.VisitStage()>=1){UE_LOG(LogTemp,Display,TEXT("S47: chain: the visit began after %.1f s standing there"),Elapsed-ChainAt);ChainAt=Elapsed;Stage=174;return true;}
     if(Elapsed-ChainAt>25)return Fail(TEXT("standing at the marks must bring the visit"));
     return true;
    }
    if(Stage==174)
    {
     // Run for a roof: the old house at the farm, any building in town.
     auto& Tk=*M->Freeform->Taken;
     if(Site.PlaceId()==TEXT("farm"))
     {
      const FVector In=S47FarmHouse::Point(2.5,2.4)+FVector(-24,0,300);M->Player->Place(In.X,In.Z,0);
      if(Tk.Indoors()&&Tk.Busy()){UE_LOG(LogTemp,Display,TEXT("S47: chain: steps overhead after running in"));Stage=175;ChainAt=Elapsed;return true;}
      if(Elapsed-ChainAt>12)return Fail(TEXT("running under a roof must start the steps overhead"));
      return true;
     }
     UE_LOG(LogTemp,Display,TEXT("S47: chain: in town, indoors %d"),Tk.Indoors());Stage=175;ChainAt=Elapsed;return true;
    }
    if(Stage==175)
    {
     // And a menu holds it: no time passes in the light behind a panel.
     auto& Tk=*M->Freeform->Taken;
     if(Tk.Busy()&&!ChainPaused){M->bPausedByMenu=true;ChainPaused=true;ChainAt=Elapsed;ChainAge=Tk.LightOn();return true;}
     if(ChainPaused&&Elapsed-ChainAt<3)return true;
     if(ChainPaused&&M->bPausedByMenu)
     {
      M->bPausedByMenu=false;
      if(Tk.LightOn()>ChainAge+.01)return Fail(TEXT("the light must not count behind a menu"));
     }
     Stage=176;ChainAt=Elapsed;return true;
    }
    UE_LOG(LogTemp,Display,TEXT("S47: chain PASS"));
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   // A look at each simplified panel.
   if(Stage>=180&&Stage<=188)
   {
    if(Elapsed-StageAt<.9)return true;
    auto& F=*M->Freeform->Field;
    const auto Close=[&]{if(M->UI)M->UI->Close(true);};
    switch(Stage)
    {
     case 180:{const FVector B=F.InteractionPoint(TEXT("ff:bench"));M->Player->Place(B.X,B.Z-1,0);F.OpenWorkshop();break;}
     case 181:Shot(TEXT("panel-workshop.png"));break;
     case 182:Close();F.OpenBranch(FS47ArrayWear::Branches()[0].Id);break;
     case 183:Shot(TEXT("panel-branch.png"));break;
     case 184:Close();M->Freeform->OpenArchive();break;
     case 185:Shot(TEXT("panel-archive.png"));break;
     case 186:Close();{const FVector T=F.TruckPointForTest();M->Player->Place(T.X,T.Z+3,0);}F.Use(TEXT("ff:cargo"));break;
     case 187:Shot(TEXT("panel-cargo.png"));break;
     case 188:Close();UE_LOG(LogTemp,Display,TEXT("S47: panels shown"));FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    ++Stage;StageAt=Elapsed;return true;
   }
   // One of them, stock still, head up, looking at the marks.
   if(Stage==190)
   {
    StageAt=Elapsed;
    auto* H=M->TownLayer&&!M->TownLayer->Herds.IsEmpty()?M->TownLayer->Herds[0].Get():nullptr;
    if(!H)return Fail(TEXT("no herd"));
    if(Elapsed-CowFrom<3)return true;
    if(H->Starer()<0)return Fail(TEXT("one cow must stand and stare at the marks"));
    FVector A;H->CowAt(H->Starer(),A);CowA=A;CowFrom=Elapsed;Stage=191;
    UE_LOG(LogTemp,Display,TEXT("S47: cow %d stares"),H->Starer());return true;
   }
   if(Stage==191)
   {
    StageAt=Elapsed;
    auto* H=M->TownLayer->Herds[0].Get();
    if(Elapsed-CowFrom<20)return true;
    if(H->Starer()<0)return Fail(TEXT("the stare must last"));
    FVector B;H->CowAt(H->Starer(),B);
    if(FVector::Dist(FVector(CowA.X,0,CowA.Z),FVector(B.X,0,B.Z))>.7)return Fail(TEXT("the staring cow must stay put"));
    UE_LOG(LogTemp,Display,TEXT("S47: stare PASS"));
    FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   if(Stage==139)
   {
    if(Elapsed-VisitFrom<2)return true;
    auto& D=*M->Freeform->Director;const FVector P=D.LightPlacePoint(TEXT("cinema"));
    if(P.IsZero())return Fail(TEXT("the cinema must be a place"));
    M->Freeform->FarmSite->Arm(M->Freeform->Clock(),TEXT("cinema"),TEXT("the Plaza cinema"),TEXT("diner"),P);
    const FVector C=M->Freeform->FarmSite->Centre();M->Player->Place(C.X,C.Z,0);
    UE_LOG(LogTemp,Display,TEXT("S47: visit test in town, site %s"),*C.ToCompactString());
    VisitClock=M->Freeform->Clock();M->Freeform->FarmSite->TestVisit();Stage=140;VisitFrom=Elapsed;StageAt=Elapsed;return true;
   }
   // Stay and it comes for you; walk off and it lets go.
   if(Stage>=140&&Stage<=146)
   {
    auto& Site=*M->Freeform->FarmSite;
    StageAt=Elapsed; // long waits by design; the visit keeps its own time
    if(Stage==140)
    {
     if(Site.VisitStage()>=2){Stage=141;VisitFrom=Elapsed;StageAt=Elapsed;return true;}
     if(Elapsed-VisitFrom>35)return Fail(TEXT("a body must come out at the site"));
     return true;
    }
    if(Stage==141)
    {
     auto Views=M->Night->Glimpses->Individuals();
     if(Views.IsEmpty())return Fail(TEXT("the visit has no body"));
     if(Elapsed-VisitFrom<9)return true;
     const FVector D=Views[0].Pos-FVector(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
     M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=-.12;
     UE_LOG(LogTemp,Display,TEXT("S47: visit body %.1f m away, state %d, rendered %d"),D.Size(),int32(Views[0].State),Views[0].bRendered);
     Stage=142;VisitFrom=Elapsed;StageAt=Elapsed;return true;
    }
    if(Stage==142){if(Elapsed-VisitFrom<1)return true;Shot(TEXT("visit-body.png"));Stage=143;VisitFrom=Elapsed;StageAt=Elapsed;return true;}
    if(Stage==143)
    {
     const double Before=VisitClock;
     if(Site.VisitStage()==4)
     {
      M->Player->TestMove.Reset();
      UE_LOG(LogTemp,Display,TEXT("S47: visit reached the player after %.1f s"),Elapsed-VisitFrom);
      if(M->Freeform->Clock()<VisitClock+14*60)return Fail(TEXT("contact must take about fifteen minutes"));
      Stage=144;VisitFrom=Elapsed;StageAt=Elapsed;return true;
     }
     (void)Before;
     // The player goes to look. It must come for them once it hunts and they are near.
     {
      const auto Views=M->Night->Glimpses->Individuals();
      if(!Views.IsEmpty()&&Site.VisitStage()==3)
      {
       const FVector D=Views[0].Pos-FVector(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
       M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=-.1;M->Player->TestMove=FVector2D(0,-.8);
       static double NextLog=0;
       if(Elapsed>=NextLog){NextLog=Elapsed+4;UE_LOG(LogTemp,Display,TEXT("S47: visit watch %.1f m, state %d, speed %.2f"),D.Size(),int32(Views[0].State),Views[0].Speed);}
      }
     }
     if(Elapsed-VisitFrom>100)return Fail(TEXT("staying must end in contact"));
     return true;
    }
    if(Stage==144)
    {
     if(Elapsed-VisitFrom<2.5)return true;
     Site.TestVisit();Stage=145;VisitFrom=Elapsed;StageAt=Elapsed;return true;
    }
    if(Stage==145)
    {
     if(Site.VisitStage()==4)return Fail(TEXT("a second body must come"));
     if(Site.VisitStage()<2)return true;
     const FVector C=Site.Centre();M->Player->Place(C.X+90,C.Z,0);Stage=146;VisitFrom=Elapsed;StageAt=Elapsed;return true;
    }
    if(Site.VisitStage()==4)
    {
     if(!M->Night->Glimpses->Individuals().IsEmpty())return Fail(TEXT("leaving must clear the body"));
     UE_LOG(LogTemp,Display,TEXT("S47: visit PASS: warned, came out, reached the player who stayed, let go of the one who left"));
     FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    if(Elapsed-VisitFrom>6)return Fail(TEXT("the visit must end when the player leaves"));
    return true;
   }
   // The service door on a far antenna, then the truck out over the field and the walk from where it stops.
   if(Stage>=120&&Stage<=132)
   {
    auto* F=M->Freeform->Field.Get();
    if(Elapsed-StageAt<(Stage==124||Stage==127?2.5:Stage==129?7.:Stage==130?5.:.8))return true;
    switch(Stage)
    {
     case 120:
     {
      const FVector Face=F->ServiceDoorFace();const double Y=F->ServiceDoorYaw();
      if(Face.IsNearlyZero())return Fail(TEXT("no service door at S-08"));
      // Stand 1.6 m out from the face, a little to the hinge side, looking at it.
      const FVector Eye=Face+FVector(-1.6*FMath::Cos(Y)+.5*FMath::Sin(Y),0,1.6*FMath::Sin(Y)+.5*FMath::Cos(Y));
      M->Player->Place(Eye.X,Eye.Z,0);
      const FVector D=Face-Eye;M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=-.25;
      break;
     }
     case 121:Shot(TEXT("field-door-shut.png"));break;
     case 122:if(!M->Freeform->Field->Use(TEXT("ff:service")))return Fail(TEXT("the service door does not take E"));break;
     case 123:if(F->ServiceStage()!=4)return Fail(TEXT("using S-08 must open the hatch"));if(F->ServiceDoorOpen()<=.05||F->ServiceDoorOpen()>=.98)UE_LOG(LogTemp,Display,TEXT("S47: door k %.2f"),F->ServiceDoorOpen());break;
     case 124:if(F->ServiceDoorOpen()<.99)return Fail(TEXT("the door must swing fully open"));Shot(TEXT("field-door-open.png"));break;
     case 125:M->Player->Place(-24,-60,0);M->World->DriveOut();break;
     case 126:
      if(!M->World->bDriving)return Fail(TEXT("not driving"));
      if(!M->World->MoveDriven(-24,-150,PI/2))return Fail(TEXT("cannot move the truck to the highway"));
      M->World->CabDrive()->bEngine=true;
      M->World->TestInput=[]{FS47DriveIn In;In.Throttle=.8;return In;};
      break;
     case 127:break;
     case 128:Shot(TEXT("field-drive.png"));break;
     case 129:
     {
      const FVector P=M->World->CabDrive()->Pos;
      UE_LOG(LogTemp,Display,TEXT("S47: field drive at %.1f %.1f"),P.X,P.Z);
      if(FMath::Abs(P.X+24)<12)return Fail(TEXT("the truck must leave the highway onto the field"));
      M->World->TestInput=[M]{FS47DriveIn In;const auto* C=M->World->CabDrive();In.Throttle=C&&C->Speed>.08?-1.:0.;return In;};
      break;
     }
     case 130:
      if(!M->World->GetOutHere(true))return Fail(TEXT("must get out in the field"));
      FieldFrom=FVector(M->Player->Pos.X,0,M->Player->Pos.Z);
      M->World->TestInput=nullptr;
      M->Player->TestMove=FVector2D(0,-1);
      break;
     case 131:Shot(TEXT("field-out.png"));break;
     case 132:
     {
      const double Moved=FVector(M->Player->Pos.X-FieldFrom.X,0,M->Player->Pos.Z-FieldFrom.Z).Size();
      M->Player->TestMove.Reset();
      if(Moved<1)return Fail(TEXT("must walk on the field after getting out"));
      UE_LOG(LogTemp,Display,TEXT("S47: field PASS: S-08 door opened with a swing, truck drove onto the field, walked %.1f m from it"),Moved);
      Stage=133;StageAt=Elapsed;return true;
     }
    }
    ++Stage;StageAt=Elapsed;return true;
   }
   // The door on the three other ways the drive boxes stand round their antennas.
   if(Stage>=133&&Stage<=134)
   {
    static const TCHAR* Dishes[3]={TEXT("S-01"),TEXT("S-02"),TEXT("S-04")};
    if(Elapsed-StageAt<1.)return true;
    auto* F=M->Freeform->Field.Get();
    if(FieldShot>=3){FPlatformMisc::RequestExitWithStatus(false,0);return false;}
    if(Stage==133)
    {
     F->TestService(Dishes[FieldShot],4);
     const FVector Face=F->ServiceDoorFace();const double Y=F->ServiceDoorYaw();
     const FVector Eye=Face+FVector(-1.7*FMath::Cos(Y)+.6*FMath::Sin(Y),0,1.7*FMath::Sin(Y)+.6*FMath::Cos(Y));
     M->Player->Place(Eye.X,Eye.Z,0);const FVector D=Face-Eye;M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=-.25;
     Stage=134;StageAt=Elapsed;return true;
    }
    Shot(*FString::Printf(TEXT("field-door-%d.png"),FieldShot));++FieldShot;Stage=133;StageAt=Elapsed;return true;
   }
   if(Stage==110)
   {
    if(Elapsed-StageAt<.5)return true;
    if(FS47TripoCreature::ShownEyeGlints()!=0)return Fail(TEXT("no eye glint past 50 m"));
    Shot(TEXT("eyes-60m.png"));
    if(M->Story)M->Story->SetClock(12*3600);
    Stage=111;StageAt=Elapsed;return true;
   }
   if(Stage==111)
   {
    if(Elapsed-StageAt<1.5)return true;
    if(FS47TripoCreature::ShownEyeGlints()!=0)return Fail(TEXT("no eye glint by day"));
    UE_LOG(LogTemp,Display,TEXT("S47: eye test PASS: night 10 and 25 m show, 60 m and daylight do not"));FPlatformMisc::RequestExitWithStatus(false,0);return false;
   }
   if(Stage==109)
   {
    if(Elapsed-StageAt<3)return true;
    if(M->Night->Glimpses->DevSpawn(1,FS47RoswellBrain::EMode::Stand)!=1)return Fail(TEXT("the eye test needs a body outdoors"));
    Stage=103;StageAt=Elapsed;return true;
   }
   if(Stage==60)
   {
    // Each place in turn: the lights start, are presented, leave when approached and leave a site.
    static const TCHAR* Places[]={TEXT("cinema"),TEXT("feed"),TEXT("church"),TEXT("gas")};
    if(TownAt>=UE_ARRAY_COUNT(Places))
    {
     UE_LOG(LogTemp,Display,TEXT("S47: town lights PASS: four places presented, left when approached, each site found and kept"));
     FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    TownPlace=Places[TownAt];auto& D=*M->Freeform->Director;const FVector P=D.LightPlacePoint(TownPlace);
    if(P.IsZero())return Fail(TEXT("a town place has no interactable to anchor on"));
    // Along the street, which is walkable ground at every one of these fronts.
    M->Player->Place(7995,P.Z+85,0);
    UE_LOG(LogTemp,Display,TEXT("S47: town test at %s floor %.2f, place %.1f %.1f %.1f"),*TownPlace,M->Player->FloorY,P.X,P.Y,P.Z);
    if(M->FieldRadio)M->FieldRadio->TestOwn();
    if(!D.ForceLights(TownPlace))return Fail(TEXT("town lights must start at each place"));
    Stage=61;StageAt=Elapsed;return true;
   }
   if(Stage==61)
   {
    // The event's own age: a paused game would otherwise pass here with lights that never faded in.
    if(M->Freeform->Director->EventAge()<14){if(Elapsed-StageAt>25)return Fail(TEXT("the town lights' event must keep running"));StageAt=StageAt+0;return true;}
    if(!M->Freeform->Director->PhysicalObjectPresented())return Fail(TEXT("the town lights must be presented"));
    const FVector A=M->Freeform->Director->EventAnchor();const FVector E(M->Player->Pos.X,M->Player->FloorY+M->Player->Eye,M->Player->Pos.Z);const FVector D=A-E;
    M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
    Stage=62;StageAt=Elapsed;return true;
   }
   if(Stage==62)
   {
    Shot(*FString::Printf(TEXT("town-lights-%s.png"),*TownPlace));Stage=66;StageAt=Elapsed;return true;
   }
   if(Stage==66)
   {
    const int32 Lit=Exposed(M);UE_LOG(LogTemp,Display,TEXT("S47: town lights at %s, bright pixels in the photograph %d"),*TownPlace,Lit);
    const FVector P=M->Freeform->Director->LightPlacePoint(TownPlace);M->Player->Place(7995,P.Z+12,0);
    Stage=63;StageAt=Elapsed;return true;
   }
   if(Stage==63)
   {
    if(!M->Freeform->Director->ActiveEvent().IsEmpty()){if(Elapsed-StageAt>12)return Fail(TEXT("the town lights must leave when approached"));return true;}
    auto& Site=*M->Freeform->FarmSite;
    if(!Site.Armed()||Site.PlaceId()!=TownPlace)return Fail(TEXT("the lights must leave a site at that place"));
    const FVector C=Site.Centre();const FVector2D At(C.X+5,C.Z+5);
    M->Player->Place(At.X,At.Y,0);
    const FVector D=C-FVector(At.X,M->Player->FloorY+1.7,At.Y);M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
    UE_LOG(LogTemp,Display,TEXT("S47: town test site %s centre %.1f %.1f %.1f floor %.2f"),*TownPlace,C.X,C.Y,C.Z,M->Player->FloorY);
    Stage=64;StageAt=Elapsed;return true;
   }
   if(Stage==64)
   {
    Shot(*FString::Printf(TEXT("town-site-%s.png"),*TownPlace));Stage=65;StageAt=Elapsed;return true;
   }
   if(Stage==65)
   {
    auto& Site=*M->Freeform->FarmSite;
    if(Site.Label().IsEmpty()||!Site.Use()||!M->Freeform->HasFinding(Site.FindingId()))return Fail(TEXT("a town site must keep one field record"));
    M->UI->Close(true);++TownAt;Stage=60;StageAt=Elapsed-.5;return true;
   }
   if(Stage==49)
   {
    SkyBefore=Exposed(M);
    if(!M->Freeform->Director->Force(TEXT("farm_lights")))return Fail(TEXT("farm lights must start at night"));
    Stage=50;StageAt=Elapsed;return true;
   }
   if(Stage==50)
   {
    if(Elapsed-StageAt<15)return true; // Faded in.
    if(!M->Freeform->Director->PhysicalObjectPresented())return Fail(TEXT("the farm lights must be presented from the yard"));
    Shot(TEXT("farm-lights.png"));Stage=51;StageAt=Elapsed;return true;
   }
   if(Stage==51)
   {
    const int32 Lit=Exposed(M);
    const auto& Claims=M->Freeform->Director->PhotoClaims();
    // The same view before and after: the lights have to be bright in the actual photograph, not just present.
    UE_LOG(LogTemp,Display,TEXT("S47: farm lights photo bright pixels %d before, %d with lights"),SkyBefore,Lit);
    if(Claims.Num()!=1||!Claims[0].Id.StartsWith(TEXT("farm_lights.")))return Fail(TEXT("a photograph from the yard must qualify the farm lights"));
    if(Lit<SkyBefore+6)return Fail(TEXT("the farm lights must actually be bright in the photograph"));
    M->Player->Place(40,450,0);Stage=52;StageAt=Elapsed;return true;
   }
   if(Stage==52)
   {
    if(!M->Freeform->Director->ActiveEvent().IsEmpty()){if(Elapsed-StageAt>10)return Fail(TEXT("the farm lights must leave when someone comes close"));return true;}
    if(!M->Freeform->FarmSite->Armed())return Fail(TEXT("the lights must leave something behind"));
    const FVector C=M->Freeform->FarmSite->Centre();const FVector2D At(C.X+7,C.Z+7);const FVector D=C-FVector(At.X,C.Y+1.7,At.Y);
    M->Player->Place(At.X,At.Y,FMath::Atan2(-D.X,-D.Z));M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size());
    Stage=53;StageAt=Elapsed;return true;
   }
   if(Stage==53)
   {
    Shot(TEXT("farm-site.png"));Stage=54;StageAt=Elapsed;return true;
   }
   if(Stage==54)
   {
    auto& Site=*M->Freeform->FarmSite;
    if(Site.Label().IsEmpty()||!Site.Use()||!M->Freeform->HasFinding(Site.FindingId()))return Fail(TEXT("looking closer must keep one field record"));
    Stage=55;StageAt=Elapsed;return true;
   }
   if(Stage==55)
   {
    Shot(TEXT("farm-site-panel.png"));Stage=56;StageAt=Elapsed;return true;
   }
   if(Stage==56)
   {
    M->UI->Close(true);auto& Site=*M->Freeform->FarmSite;
    const auto Saved=Site.Save();Site.Begin(Saved);if(!Site.Armed())return Fail(TEXT("the site must survive a reload"));
    if(!M->Freeform->TestAdvance(16.*3600.))return Fail(TEXT("advancing past noon"));
    Stage=57;StageAt=Elapsed;return true;
   }
   if(Stage==57)
   {
    if(M->Freeform->FarmSite->Armed())return Fail(TEXT("the site must be gone by the next noon"));
    UE_LOG(LogTemp,Display,TEXT("S47: farm lights PASS: bright in the photograph, left when approached, site found, kept, reloaded and gone by morning"));
    FPlatformMisc::RequestExitWithStatus(false,0);
    return false;
   }
   if(Stage==10)
   {
    if(!Sees(TEXT("Album")))return Fail(TEXT("the notebook did not open on the Album"));
    Shot(TEXT("album-tab.png"));Press(EKeys::Enter);Stage=11;StageAt=Elapsed;return true;
   }
   if(Stage==11)
   {
    if(!Sees(TEXT("X  THROW AWAY PHOTO")))return Fail(TEXT("an open ordinary print offers no way to throw it away"));
    Shot(TEXT("album-print.png"));Press(EKeys::X);Stage=12;StageAt=Elapsed;return true;
   }
   if(Stage==12)
   {
    if(!Sees(TEXT("X  AGAIN TO THROW IT AWAY"))||M->Night->Album->Shots().Num()!=2)return Fail(TEXT("the first X must only ask"));
    Shot(TEXT("album-armed.png"));Press(EKeys::X);Stage=13;StageAt=Elapsed;return true;
   }
   if(Stage==13)
   {
    if(M->Night->Album->Shots().Num()!=1||M->Night->Album->Left()!=LeftBefore||!M->ModalOpen())return Fail(TEXT("the second X must remove the print and keep its frame used"));
    Shot(TEXT("album-after.png"));Press(EKeys::Enter);Stage=14;StageAt=Elapsed;return true;
   }
   if(Stage==14)
   {
    if(!Sees(TEXT("KEPT: USED IN YOUR WORK")))return Fail(TEXT("a print the game saw something in must be kept"));
    Shot(TEXT("album-kept.png"));Press(EKeys::X);Press(EKeys::X);Stage=15;StageAt=Elapsed;return true;
   }
   if(Stage==15)
   {
    if(M->Night->Album->Shots().Num()!=1)return Fail(TEXT("a kept print was thrown away"));
    UE_LOG(LogTemp,Display,TEXT("S47: album PASS: tab, open print, two-step throw away, frame stays used, kept print refused"));
    M->UI->Close(true);
    const auto& Decor=M->Freeform->Decor;const FString Kept=M->Night->Album->Shots()[0].Id;
    if(!Decor||!Decor->Hang(1,Kept)||!Decor->HoldsPhoto(Kept))return Fail(TEXT("a developed print must go up in a frame"));
    const TArray<FString> Saved=S47J::Strs(Decor->Save(),TEXT("frames"));
    if(Saved.Num()!=FS47RoomDecor::SlotCount||Saved[1]!=Kept)return Fail(TEXT("the frame must save which print it holds"));
    M->Player->Place(-3.4,2.2,PI/2);Stage=20;StageAt=Elapsed;return true;
   }
   if(Stage==20)
   {
    Shot(TEXT("decor-wall.png"));Stage=22;StageAt=Elapsed;return true;
   }
   if(Stage==22)
   {
    M->Freeform->Decor->Use(TEXT("ff:decor:0"));Stage=23;StageAt=Elapsed;return true;
   }
   if(Stage==23)
   {
    Shot(TEXT("decor-panel.png"));Stage=21;StageAt=Elapsed;return true;
   }
   if(Stage==21)
   {
    M->UI->Close(true);
    // A reload puts the same print back; a print that no longer exists leaves the frame empty.
    const auto Saved=M->Freeform->Decor->Save();const FString Kept=M->Freeform->Decor->SlotPhoto(1);
    M->Freeform->Decor->Begin(Saved);M->Freeform->Decor->Step();
    if(M->Freeform->Decor->SlotPhoto(1)!=Kept)return Fail(TEXT("a reloaded frame must hold the same print"));
    const auto Gone=S47J::New();S47J::Set(Gone,TEXT("frames"),TArray<FString>{TEXT("no-such-print"),Kept});
    M->Freeform->Decor->Begin(Gone);M->Freeform->Decor->Step();
    if(!M->Freeform->Decor->SlotPhoto(0).IsEmpty())return Fail(TEXT("a missing print must leave its frame empty"));
    UE_LOG(LogTemp,Display,TEXT("S47: room decor PASS: hang, save, reload, missing print"));
    S47Panels::WetBench(M->UI,TEXT("SARO / PHOTO LAB  |  FILM: free1067653  |  Control room"),0,FString(),[]{});Stage=16;StageAt=Elapsed;return true;
   }
   if(Stage==16)
   {
    Shot(TEXT("wetbench.png"));M->UI->Close(true);
    M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO / RECORDS"),S47Kit::Col({
     S47Kit::Choice(TEXT("Logger battery pack"),true,[]{},false,TEXT("DAY 1")),
     S47Kit::Choice(TEXT("D01-01 / ORIGINAL D01-01-original"),false,[]{},false,TEXT("COMPARISON PENDING / RECORDED")),
     S47Kit::Paper(TEXT("DAY 1\nRETAINED RECORD / Logger battery pack built at the SARO bench from tested donor parts."))}),1000));
    Stage=18;StageAt=Elapsed;return true;
   }
   if(Stage==18)
   {
    Shot(TEXT("records.png"));Stage=17;StageAt=Elapsed;return true;
   }
   if(Stage==17)
   {
    FPlatformMisc::RequestExitWithStatus(false,0);
    return false;
   }
   // QUIT must exit through the ordinary menu callback. A swallowed click times out above.
   return true;
  }
 };
}

void S47MenuPointerTest::Start(US47Main* Main)
{
 if(!FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerTest"))) return;
 FString User;
 FParse::Value(FCommandLine::Get(),TEXT("UserDir="),User);
 if(!User.StartsWith(TEXT("/tmp/S47-menu-pointer-")) || !FParse::Param(FCommandLine::Get(),TEXT("S47Title")))
 {
  UE_LOG(LogTemp,Error,TEXT("S47: menu pointer FAIL: requires -S47Title and isolated /tmp/S47-menu-pointer-* UserDir"));
  FPlatformMisc::RequestExitWithStatus(false,1);
  return;
 }
 const auto Run=MakeShared<FRun>();Run->Main=Main;
 Run->bLethal=FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerLethal"));
 Run->bQuit=FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerQuit"));
 Run->bFullCases=FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerFullCases"));
 Run->bAlbum=FParse::Param(FCommandLine::Get(),TEXT("S47MenuPointerAlbum"));
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float Dt){return Run->Step(Dt);}));
}

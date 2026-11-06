#include "S47MenuPointerTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Album.h"
#include "S47Flow.h"
#include "S47Night.h"
#include "S47Panels.h"
#include "S47Kit.h"
#include "S47FarmHouse.h"
#include "S47World.h"
#include "S47Inter.h"
#include "S47FreeformDirector.h"
#include "S47FarmSite.h"
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
  bool ShotPending=false;TArray<FVector> WalkTo;int32 WalkAt=0;double WalkFrom=0;
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
   if(Stage!=77&&Elapsed-StageAt<1) return true;
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
    // Actually walked: in by the front door, down the hall, out of the back. Stopped by any wall on the way.
    const FVector O(-24,0,300);WalkTo={S47FarmHouse::Point(6,9.6)+O,S47FarmHouse::Point(6,4.5)+O,S47FarmHouse::Point(6,-1)+O};WalkAt=0;
    M->Player->Place(WalkTo[0].X,WalkTo[0].Z+2,0);Stage=77;StageAt=Elapsed;WalkFrom=Elapsed;return true;
   }
   if(Stage==77)
   {
    StageAt=Elapsed;
    if(Elapsed-WalkFrom>30)return Fail(TEXT("the hall must be walkable from the front door to the back door"));
    if(WalkAt>=WalkTo.Num())
    {
     M->Player->TestMove.Reset();
     UE_LOG(LogTemp,Display,TEXT("S47: farmhouse PASS: level floor, walked in the front door and out of the back in %.1f s"),Elapsed-WalkFrom);
     FPlatformMisc::RequestExitWithStatus(false,0);return false;
    }
    const FVector V=WalkTo[WalkAt]-M->Player->Pos;const double Left=FVector2D(V.X,V.Z).Size();
    if(Left<.3){++WalkAt;return true;}
    M->Player->Yaw=FMath::Atan2(-V.X,-V.Z);M->Player->Pitch=-.08;M->Player->TestMove=FVector2D(0,-FMath::Min(1.,Left/.4));
    return true;
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
    if(!M->Freeform->TestAdvance(10.*3600.))return Fail(TEXT("advancing to morning"));
    Stage=57;StageAt=Elapsed;return true;
   }
   if(Stage==57)
   {
    if(M->Freeform->FarmSite->Armed())return Fail(TEXT("the site must be gone by morning"));
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

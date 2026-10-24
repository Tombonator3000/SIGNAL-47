#include "S47Gamepad.h"
#include "S47Startup.h"
#include "Misc/CommandLine.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Widgets/SWindow.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Gamepad, Log, All);
namespace
{
 struct FPad : IInputProcessor
 {
  TWeakObjectPtr<APlayerController> PC;
  FVector2D Left = FVector2D::ZeroVector, Right = FVector2D::ZeroVector;
  double LT=0, RT=0;
  TSet<FKey> Held, Pending, Edges, Suppressed;
  TMap<FKey,FKey> UiDownKeys;
  uint64 Frame=MAX_uint64;
  bool bPad=false, bUi=false, bPointer=false, bPointerDown=false, bDispatch=false;
  uint32 Revision=0;
  bool bActive=true, bBackgroundTest=false, WaitLeft=false, WaitRight=false, WaitLT=false, WaitRT=false;
  bool AcceptInput() const {return bActive||bBackgroundTest;}
  FDelegateHandle Activation, Connection;
  FInputDeviceId Owner;
  FVector2D VirtualPointer=FVector2D::ZeroVector;bool bHavePointer=false;
  FVector2D PointerAt(FSlateApplication& App) const {return bBackgroundTest&&bHavePointer?VirtualPointer:App.GetCursorPos();}
  void Device(bool Pad)
  {
   if(!Pad && bPointerDown)CancelPointer(FSlateApplication::Get());
   if (bPad!=Pad) { bPad=Pad; ++Revision; UE_LOG(LogS47Gamepad,Display,TEXT("S47: input device %s"),Pad?TEXT("xbox"):TEXT("keyboard-mouse")); }
  }
  bool Menu() const { return PC.IsValid() && PC->bShowMouseCursor; }
  void PointerButton(FSlateApplication& App,bool Down)
  {
   if (bPointerDown==Down) return;
   bPointerDown=Down;
   const FVector2D At=PointerAt(App);
   TSet<FKey> Buttons=App.GetPressedMouseButtons();
   if (Down) Buttons.Add(EKeys::LeftMouseButton); else Buttons.Remove(EKeys::LeftMouseButton);
   const FPointerEvent Event(0,At,At,Buttons,EKeys::LeftMouseButton,0,App.GetModifierKeys());
   TGuardValue<bool> Guard(bDispatch,true);
   if (Down) App.ProcessMouseButtonDownEvent(nullptr,Event); else App.ProcessMouseButtonUpEvent(Event);
  }
  void CancelPointer(FSlateApplication& App)
  {
   if(!bPointerDown)return;
   // Capture loss cancels board/card drags; only the actual A release may commit a drop.
   App.ReleaseAllPointerCapture(0);PointerButton(App,false);
  }
  void ReleaseUiKeys(FSlateApplication& App)
  {
   TMap<FKey,FKey> Keys=MoveTemp(UiDownKeys);UiDownKeys.Reset();
   TGuardValue<bool> Guard(bDispatch,true);
   for(const auto& Pair:Keys)App.ProcessKeyUpEvent(FKeyEvent(Pair.Value,App.GetModifierKeys(),0,false,0,0));
  }
  void Clear(FSlateApplication& App)
  {
   ReleaseUiKeys(App);CancelPointer(App); Left=Right=FVector2D::ZeroVector; LT=RT=0;
   Held.Reset(); Pending.Reset(); Edges.Reset(); Suppressed.Reset(); bPointer=false;
   WaitLeft=WaitRight=WaitLT=WaitRT=false;
  }
  void SyncMenu(FSlateApplication& App)
  {
   if (bUi==Menu())return;
   if(bUi&&!Menu())
   {
    Suppressed=Held; WaitLeft=!S47Gamepad::DeadZone(Left).IsNearlyZero(); WaitRight=!S47Gamepad::DeadZone(Right).IsNearlyZero();
    WaitLT=LT>.1;WaitRT=RT>.1;
   }
   CancelPointer(App); ReleaseUiKeys(App); bUi=Menu(); bPointer=false; bHavePointer=false; Pending.Reset(); Edges.Reset();
  }
  bool Adopt(const FInputEvent& E,bool Significant)
  {
   if (!Owner.IsValid() || (Owner!=E.GetInputDeviceId() && Significant))
   {
    if (Owner.IsValid()) Clear(FSlateApplication::Get());
    Owner=E.GetInputDeviceId();
   }
   return Owner==E.GetInputDeviceId();
  }
  bool TriggerBlocked(FKey K) const {return (K==EKeys::Gamepad_LeftTrigger&&WaitLT)||(K==EKeys::Gamepad_RightTrigger&&WaitRT);}
  void Button(FKey K,bool IsDown,bool Repeat=false)
  {
   if (IsDown)
   {
    if (!Held.Contains(K) && !Suppressed.Contains(K) && !TriggerBlocked(K) && !Repeat && !Menu() && !S47Startup::Blocking()) Pending.Add(K);
    Held.Add(K);
   }
   else {Held.Remove(K);Suppressed.Remove(K);}
  }
  void EdgeFrame()
  {
   if(Frame==GFrameCounter)return;
   Frame=GFrameCounter; Edges=MoveTemp(Pending); Pending.Reset();
  }
  virtual bool HandleKeyDownEvent(FSlateApplication& App,const FKeyEvent& E) override
  {
   if(bDispatch)return false;
   SyncMenu(App);
   const FKey K=E.GetKey();
   if(!K.IsGamepadKey()) { if(AcceptInput()){Device(false); bPointer=false;} return false; }
   if(!AcceptInput())return true;
   // Linux emits digital trigger edges at a much lower threshold than gameplay. Use only the analog hysteresis.
   if(K==EKeys::Gamepad_LeftTrigger||K==EKeys::Gamepad_RightTrigger)return true;
   if(!Adopt(E,true))return false;
   Device(true); Button(K,true,E.IsRepeat());
   if(!Menu())return false;
   if(K==EKeys::Gamepad_DPad_Up||K==EKeys::Gamepad_DPad_Down||K==EKeys::Gamepad_DPad_Left||K==EKeys::Gamepad_DPad_Right)bPointer=false;
   if(K==EKeys::Gamepad_FaceButton_Bottom && bPointer) { if(!E.IsRepeat())PointerButton(App,true); return true; }
   if(K==EKeys::Gamepad_FaceButton_Right)CancelPointer(App);
   // Route all controllers to the single player's Slate user. B falls back to Escape for
   // legacy panels that do not implement their own Xbox handler (including text-entry panels).
   TGuardValue<bool> Guard(bDispatch,true);
   const FKey UiKey=K==EKeys::Gamepad_Special_Right ? EKeys::Escape : K;
   const bool Handled=App.ProcessKeyDownEvent(FKeyEvent(UiKey,E.GetModifierKeys(),0,E.IsRepeat(),0,0));
   UiDownKeys.Add(K,UiKey);
   if(!Handled && K==EKeys::Gamepad_FaceButton_Right)
   {
    App.ProcessKeyUpEvent(FKeyEvent(UiKey,E.GetModifierKeys(),0,false,0,0));
    App.ProcessKeyDownEvent(FKeyEvent(EKeys::Escape,E.GetModifierKeys(),0,E.IsRepeat(),0,0));
    UiDownKeys.Add(K,EKeys::Escape);
   }
   return true;
  }
  virtual bool HandleKeyUpEvent(FSlateApplication& App,const FKeyEvent& E) override
  {
   if(bDispatch||!E.GetKey().IsGamepadKey())return false;
   if(E.GetKey()==EKeys::Gamepad_LeftTrigger||E.GetKey()==EKeys::Gamepad_RightTrigger)return true;
   if(Owner.IsValid()&&Owner!=E.GetInputDeviceId())return false;
   Button(E.GetKey(),false);
   if(const FKey* Forwarded=UiDownKeys.Find(E.GetKey()))
   {const FKey K=*Forwarded;UiDownKeys.Remove(E.GetKey());TGuardValue<bool> Guard(bDispatch,true);App.ProcessKeyUpEvent(FKeyEvent(K,E.GetModifierKeys(),0,false,0,0));return true;}
   if(E.GetKey()==EKeys::Gamepad_FaceButton_Bottom&&bPointerDown) { PointerButton(App,false); return true; }
   if(!Menu())return false;
   TGuardValue<bool> Guard(bDispatch,true);
   App.ProcessKeyUpEvent(FKeyEvent(E.GetKey(),E.GetModifierKeys(),0,false,0,0));return true;
  }
  virtual bool HandleAnalogInputEvent(FSlateApplication& App,const FAnalogInputEvent& E) override
  {
   if(bDispatch)return false;
   SyncMenu(App);
   const FKey K=E.GetKey();const float V=E.GetAnalogValue();
   if(FParse::Param(FCommandLine::Get(),TEXT("S47GamepadLog")) &&
      ((K==EKeys::Gamepad_LeftTriggerAxis&&!FMath::IsNearlyEqual(LT,double(V),.001)) ||
       (K==EKeys::Gamepad_RightTriggerAxis&&!FMath::IsNearlyEqual(RT,double(V),.001))))
    UE_LOG(LogS47Gamepad,Display,TEXT("S47: pad trigger key=%s value=%.4f ui=%d active=%d waitLT=%d waitRT=%d device=%d"),*K.ToString(),V,Menu()?1:0,AcceptInput()?1:0,WaitLT?1:0,WaitRT?1:0,E.GetInputDeviceId().GetId());
   if(!K.IsGamepadKey())return false;
   if(!AcceptInput())return true;
   if(!Adopt(E,FMath::Abs(V)>.20f))return false;
   if(S47Startup::Blocking())return false;
   if(K==EKeys::Gamepad_LeftX)Left.X=V;
   else if(K==EKeys::Gamepad_LeftY)Left.Y=V;
   else if(K==EKeys::Gamepad_RightX)Right.X=V;
   else if(K==EKeys::Gamepad_RightY)Right.Y=V;
   else if(K==EKeys::Gamepad_LeftTriggerAxis) { LT=V; Button(EKeys::Gamepad_LeftTrigger,V>(Held.Contains(EKeys::Gamepad_LeftTrigger)?.40f:.55f)); }
   else if(K==EKeys::Gamepad_RightTriggerAxis) { RT=V; Button(EKeys::Gamepad_RightTrigger,V>(Held.Contains(EKeys::Gamepad_RightTrigger)?.40f:.55f)); }
   if(S47Gamepad::DeadZone(Left).IsNearlyZero())WaitLeft=false;
   if(S47Gamepad::DeadZone(Right).IsNearlyZero())WaitRight=false;
   if(LT<=.1)WaitLT=false;if(RT<=.1)WaitRT=false;
   if(FMath::Abs(V)>.20f)Device(true);
   if(!Menu())return false;
   if(K==EKeys::Gamepad_RightX||K==EKeys::Gamepad_RightY)return true;
   if((K==EKeys::Gamepad_LeftX||K==EKeys::Gamepad_LeftY)&&FMath::Abs(V)>.5f)bPointer=false;
   TGuardValue<bool> Guard(bDispatch,true);
   App.ProcessAnalogInputEvent(FAnalogInputEvent(K,E.GetModifierKeys(),0,false,0,0,V));return true;
  }
  virtual bool HandleMouseMoveEvent(FSlateApplication&,const FPointerEvent& E) override
  { if(!bDispatch && E.GetCursorDelta().SizeSquared()>1) { Device(false); bPointer=false; }return false; }
  virtual bool HandleMouseButtonDownEvent(FSlateApplication&,const FPointerEvent&) override
  { if(!bDispatch) { Device(false); bPointer=false; }return false; }
  virtual void Tick(float Dt,FSlateApplication& App,TSharedRef<ICursor>) override
  {
   SyncMenu(App);
   if(!AcceptInput()||!Menu()||S47Startup::Blocking())return;
   const FVector2D Stick=S47Gamepad::DeadZone(Right,.22);
   if(!Stick.IsNearlyZero())
   {
    bPointer=true;Device(true);
    const TSharedPtr<SWindow> W=GEngine&&GEngine->GameViewport?GEngine->GameViewport->GetWindow():nullptr;
    if(W.IsValid())
    {
     const FSlateRect R=W->GetClientRectInScreen();
     FVector2D Old=PointerAt(App);
     if(Old.X<R.Left||Old.X>R.Right||Old.Y<R.Top||Old.Y>R.Bottom)Old=FVector2D((R.Left+R.Right)*.5,(R.Top+R.Bottom)*.5);
     const double Speed=FMath::Max(650.0,double(R.Right-R.Left)*.55);
     FVector2D At=Old+FVector2D(Stick.X,-Stick.Y)*Speed*FMath::Min(Dt,.05f);
     At.X=FMath::Clamp(At.X,double(R.Left+2),double(R.Right-2));At.Y=FMath::Clamp(At.Y,double(R.Top+2),double(R.Bottom-2));
     TGuardValue<bool> Guard(bDispatch,true);
     VirtualPointer=At;bHavePointer=true;
     // Diagnostic offscreen tests route a logical pointer without moving the desktop cursor.
     if(!bBackgroundTest)App.SetCursorPos(At);
     TSet<FKey> Buttons=App.GetPressedMouseButtons();if(bPointerDown)Buttons.Add(EKeys::LeftMouseButton);
     App.ProcessMouseMoveEvent(FPointerEvent(0,At,Old,Buttons,EKeys::Invalid,0,App.GetModifierKeys()));
    }
   }
   const double Scroll=(LT>.15?LT:0)-(RT>.15?RT:0);
   if(FMath::Abs(Scroll)>.01)
   {
    const FVector2D At=PointerAt(App);TGuardValue<bool> Guard(bDispatch,true);
    App.ProcessMouseWheelOrGestureEvent(FPointerEvent(0,At,At,App.GetPressedMouseButtons(),EKeys::Invalid,Scroll*Dt*6,App.GetModifierKeys()),nullptr);
   }
  }
 };
 TSharedPtr<FPad> Pad;
}
FVector2D S47Gamepad::DeadZone(FVector2D V,double Zone)
{
 const double L=V.Size();if(!FMath::IsFinite(L)||L<=Zone)return FVector2D::ZeroVector;
 return V/L*FMath::Clamp((L-Zone)/(1-Zone),0.0,1.0);
}
void S47Gamepad::Init(APlayerController* PC)
{
 Shutdown();if(!FSlateApplication::IsInitialized())return;
 Pad=MakeShared<FPad>();Pad->PC=PC;Pad->bActive=FSlateApplication::Get().IsActive();
 Pad->bBackgroundTest=FParse::Param(FCommandLine::Get(),TEXT("RenderOffscreen"))&&FParse::Param(FCommandLine::Get(),TEXT("S47GamepadBackgroundTest"));
 FSlateApplication::Get().RegisterInputPreProcessor(Pad,0);
 Pad->Activation=FSlateApplication::Get().OnApplicationActivationStateChanged().AddLambda([](bool Active){if(Pad.IsValid()){Pad->bActive=Active;if(!Active)Pad->Clear(FSlateApplication::Get());}});
 Pad->Connection=IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().AddLambda([](EInputDeviceConnectionState State,FPlatformUserId,FInputDeviceId Id){
  if(Pad.IsValid()&&State==EInputDeviceConnectionState::Disconnected&&Pad->Owner==Id)
  {Pad->Clear(FSlateApplication::Get());Pad->Owner=FInputDeviceId();UE_LOG(LogS47Gamepad,Display,TEXT("S47: gamepad disconnected; axes and held buttons cleared"));}
 });
 UE_LOG(LogS47Gamepad,Display,TEXT("S47: Xbox input ready; radial dead zone 0.18; right-stick UI pointer"));
}
void S47Gamepad::Shutdown()
{
 if(!Pad.IsValid())return;
 if(FSlateApplication::IsInitialized()) {Pad->Clear(FSlateApplication::Get());FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(Pad->Activation);FSlateApplication::Get().UnregisterInputPreProcessor(Pad);}
 IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().Remove(Pad->Connection);Pad.Reset();
}
bool S47Gamepad::UsingGamepad(){return Pad.IsValid()&&Pad->bPad;}
uint32 S47Gamepad::Changes(){return Pad.IsValid()?Pad->Revision:0;}
FKey S47Gamepad::Key(FName A)
{
 static const TMap<FName,FKey> Map={
 {TEXT("use"),EKeys::Gamepad_FaceButton_Bottom},{TEXT("alt"),EKeys::Gamepad_FaceButton_Right},
 {TEXT("camera"),EKeys::Gamepad_FaceButton_Left},{TEXT("jump"),EKeys::Gamepad_FaceButton_Top},
 {TEXT("run"),EKeys::Gamepad_LeftThumbstick},{TEXT("zoom"),EKeys::Gamepad_LeftTrigger},
 {TEXT("shutter"),EKeys::Gamepad_RightTrigger},{TEXT("light"),EKeys::Gamepad_DPad_Up},
 {TEXT("radio"),EKeys::Gamepad_DPad_Down},{TEXT("map"),EKeys::Gamepad_DPad_Left},
 {TEXT("describe"),EKeys::Gamepad_DPad_Right},{TEXT("notebook"),EKeys::Gamepad_Special_Left},
 {TEXT("board"),EKeys::Gamepad_RightShoulder},{TEXT("pause"),EKeys::Gamepad_Special_Right}};
 const FKey* K=Map.Find(A);return K?*K:EKeys::Invalid;
}
FString S47Gamepad::Label(FName A)
{
 static const TMap<FName,FString> Map={
 {TEXT("forward"),TEXT("LS UP")},{TEXT("back"),TEXT("LS DOWN")},{TEXT("left"),TEXT("LS LEFT")},{TEXT("right"),TEXT("LS RIGHT")},
 {TEXT("use"),TEXT("A")},{TEXT("alt"),TEXT("B")},{TEXT("camera"),TEXT("X")},{TEXT("jump"),TEXT("Y")},
 {TEXT("run"),TEXT("LS CLICK")},{TEXT("zoom"),TEXT("LT")},{TEXT("shutter"),TEXT("RT")},{TEXT("light"),TEXT("DPAD UP")},
 {TEXT("radio"),TEXT("DPAD DOWN")},{TEXT("map"),TEXT("DPAD LEFT")},{TEXT("describe"),TEXT("DPAD RIGHT")},
 {TEXT("notebook"),TEXT("VIEW / LB")},{TEXT("board"),TEXT("RB")},{TEXT("pause"),TEXT("MENU")}};
 const FString* S=Map.Find(A);return S?*S:FString();
}
bool S47Gamepad::Down(const FKey& K){if(!Pad.IsValid()||!Pad->AcceptInput())return false;Pad->SyncMenu(FSlateApplication::Get());return !Pad->Suppressed.Contains(K)&&!Pad->TriggerBlocked(K)&&Pad->Held.Contains(K);}
bool S47Gamepad::Pressed(const FKey& K){if(!Pad.IsValid()||!Pad->AcceptInput())return false;Pad->SyncMenu(FSlateApplication::Get());Pad->EdgeFrame();return !Pad->TriggerBlocked(K)&&Pad->Edges.Contains(K);}
FVector2D S47Gamepad::Move(){if(Pad.IsValid())Pad->SyncMenu(FSlateApplication::Get());const FVector2D V=Pad.IsValid()&&Pad->AcceptInput()&&!Pad->WaitLeft?DeadZone(Pad->Left):FVector2D::ZeroVector;return FVector2D(V.X,-V.Y);}
FVector2D S47Gamepad::Look(double Dt,double Speed,bool Invert)
{
 if(Pad.IsValid())Pad->SyncMenu(FSlateApplication::Get());const FVector2D V=Pad.IsValid()&&Pad->AcceptInput()&&!Pad->WaitRight?DeadZone(Pad->Right):FVector2D::ZeroVector;
 // Squared response preserves fine aiming near centre, with about 140 degrees/sec at full travel.
 const FVector2D C=V*V.Size();return FVector2D(C.X,Invert?C.Y:-C.Y)*2.45*FMath::Clamp(Speed,.3,2.5)*FMath::Min(Dt,.1);
}
FVector2D S47Gamepad::Drive()
{
 if(!Pad.IsValid()||!Pad->AcceptInput())return FVector2D::ZeroVector;
 Pad->SyncMenu(FSlateApplication::Get());
 const auto Trigger=[](double V){return FMath::Clamp((V-.06)/.94,0.0,1.0);};
 return FVector2D(Pad->WaitLeft?0:DeadZone(FVector2D(Pad->Left.X,0)).X,Trigger(Pad->WaitRT?0:Pad->RT)-Trigger(Pad->WaitLT?0:Pad->LT));
}
bool S47Gamepad::SelfTest(TArray<FString>& Out)
{
 bool Ok=true;auto Check=[&](bool Pass,const TCHAR* Name){Out.Add(FString::Printf(TEXT("%s xbox %s"),Pass?TEXT("OK"):TEXT("FAIL"),Name));Ok&=Pass;};
 Check(DeadZone(FVector2D(.071,.059)).IsNearlyZero(),TEXT("real virtual-pad neutral drift is ignored"));
 Check(DeadZone(FVector2D(.1,.1)).IsNearlyZero(),TEXT("diagonal centre dead zone"));
 Check(FMath::IsNearlyEqual(DeadZone(FVector2D(.59,0)).X,.5),TEXT("half-speed walking retained"));
 Check(FMath::IsNearlyEqual(DeadZone(FVector2D(1,1)).Size(),1.),TEXT("diagonal speed bounded"));
 Check(Key(TEXT("use"))==EKeys::Gamepad_FaceButton_Bottom&&Key(TEXT("jump"))==EKeys::Gamepad_FaceButton_Top,TEXT("use and jump distinct"));
 Check(Key(TEXT("shutter"))==EKeys::Gamepad_RightTrigger&&Key(TEXT("zoom"))==EKeys::Gamepad_LeftTrigger,TEXT("camera triggers"));
 if(Pad.IsValid())
 {
  TGuardValue<bool> Active(Pad->bActive,true);
  Pad->Left=FVector2D(0,1);Check(Move().Y==-1,TEXT("stick up walks forwards"));
  Pad->Left=FVector2D(.1,1);Pad->RT=Pad->LT=0;Check(Drive().IsNearlyZero(),TEXT("unused vertical stick cannot amplify steering drift"));
  Pad->RT=1;Pad->LT=0;Check(Drive().Y==1,TEXT("RT drives forwards"));
  Pad->RT=0;Pad->LT=1;Check(Drive().Y==-1,TEXT("LT brakes and reverses"));
  Pad->Right=FVector2D(1,1);const FVector2D A=Look(1./60,1,false),B=Look(1./30,1,false);
  Check((A*2-B).IsNearlyZero(),TEXT("look independent of frame rate"));
  Check(Look(.016,1,false).Y==-Look(.016,1,true).Y,TEXT("separate pad invert"));
  Pad->Held.Add(EKeys::Gamepad_FaceButton_Bottom);Pad->Clear(FSlateApplication::Get());
  Check(Move().IsNearlyZero()&&Look(.016,1,false).IsNearlyZero()&&Drive().IsNearlyZero()&&!Down(EKeys::Gamepad_FaceButton_Bottom),TEXT("disconnect and focus loss clear held input"));
 }
 if(Pad.IsValid())
 {
  TGuardValue<bool> Active(Pad->bActive,true);
  Pad->WaitRT=true;Pad->Held.Add(EKeys::Gamepad_RightTrigger);Pad->Suppressed.Add(EKeys::Gamepad_RightTrigger);
  Pad->Button(EKeys::Gamepad_RightTrigger,false);Pad->Button(EKeys::Gamepad_RightTrigger,true);
  Check(!Down(EKeys::Gamepad_RightTrigger)&&!Pad->Pending.Contains(EKeys::Gamepad_RightTrigger),TEXT("UI trigger must return fully neutral before shutter"));
  Pad->Clear(FSlateApplication::Get());
 }
 return Ok;
}

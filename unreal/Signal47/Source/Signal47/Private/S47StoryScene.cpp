#include "S47StoryScene.h"
#include "S47Main.h"
#include "S47UI.h"
#include "S47Kit.h"
#include "S47Web.h"
#include "S47Audio.h"
#include "S47DaleVoice.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47StoryScene,Log,All);

namespace S47StorySceneLocal
{
 // A closed walkie or radio scene comes back after this long (the conversation is not dropped; the phone waits to be picked up).
 constexpr double ReopenAfter=15;
 FString TitleOf(const FString& Channel)
 {
  return Channel==TEXT("phone")?TEXT("TELEPHONE"):Channel==TEXT("walkie")?TEXT("WALKIE-TALKIE"):Channel==TEXT("radio")?TEXT("RADIO"):TEXT("CONVERSATION");
 }
 bool ValidFlag(const FString& Id)
 {
  if(Id.IsEmpty()||Id.Len()>48)return false;
  for(const TCHAR C:Id)if(!((C>='a'&&C<='z')||(C>='0'&&C<='9')||C=='_'))return false;
  return true;
 }
}

FS47StoryScene::FS47StoryScene(US47Main* Main):M(Main)
{
 FString Error;
 bReady=Pack.Initialize(Error);
 if(!bReady)UE_LOG(LogS47StoryScene,Warning,TEXT("S47: story scene: the pack did not load (%s); scene beats are left out"),*Error);
}

FS47StoryScene::~FS47StoryScene()
{
 Life.Reset();
 StopRing();
 if(Voice)Voice->Stop();
}

bool FS47StoryScene::Known(const FString& Id)
{
 static FS47EveningPack Lookup;
 static bool bRead=false,bOk=false;
 if(!bRead){FString Error;bOk=Lookup.Initialize(Error);bRead=true;}
 return bOk&&Lookup.FindScene(Id)!=nullptr;
}

bool FS47StoryScene::Begin(const FString& Id,FString& Why)
{
 if(!bReady){Why=TEXT("the pack did not load");return false;}
 if(Pack.CurrentScene()){Why=TEXT("scene ")+Active+TEXT(" is under way");return false;}
 const FS47EveningScene* S=Pack.FindScene(Id);
 if(!S){Why=TEXT("no scene ")+Id;return false;}
 if(S->Entry!=TEXT("trigger")){Why=Id+TEXT(" is not a trigger scene (it follows another)");return false;}
 if(!Pack.SetPhase(S->Phase)){Why=TEXT("phase ")+S->Phase+TEXT(" could not be set");return false;}
 if(!Pack.CanStart(Id)){Why=Id+TEXT(": the pack's requirements are not met");return false;}
 if(!Pack.Start(Id)){Why=Id+TEXT(" did not start");return false;}
 Active=Id;Channel=S->Channel;SpokenLine.Reset();
 UE_LOG(LogS47StoryScene,Display,TEXT("S47: story scene %s on %s"),*Id,*Channel);
 AfterStep();
 return true;
}

void FS47StoryScene::SetContext(const TMap<FString,bool>& Ctx)
{
 for(const TPair<FString,bool>& P:Ctx)Pack.SetExternal(P.Key,P.Value);
}

bool FS47StoryScene::Busy() const
{
 return Pack.CurrentScene()!=nullptr;
}

void FS47StoryScene::StartRing()
{
 if(bRing)return;
 bRing=true;
 if(M&&M->Audio&&M->Web)M->Audio->Loop(TEXT("story:ring"),TEXT("phone_ring"),.8f,M->Web->PropLocation(TEXT("phone")));
 if(M&&M->Web)M->Web->SetHandsetRattle(true);
}

void FS47StoryScene::StopRing()
{
 if(bRing&&M&&M->Audio)M->Audio->Stop(TEXT("story:ring"),.05f);
 bRing=false;
 if(M&&M->Web)M->Web->SetHandsetRattle(false);
}

bool FS47StoryScene::UsePhone()
{
 if(!Pack.CurrentScene()||Channel!=TEXT("phone"))return false;
 if(bOpen)return true;
 StopRing();
 if(M&&M->Audio&&M->Web){M->Audio->Play(TEXT("handset_up"),.5f,M->Web->PropLocation(TEXT("phone")));M->Web->SetHandsetVisible(false);}
 Show();
 return true;
}

void FS47StoryScene::Tick(double Dt)
{
 if(!Pack.CurrentScene())return;
 // A call nobody has answered yet, whose reason has gone (the power came back before the phone was picked up): it stops
 // ringing and is taken back without its effects, so it does not ring on about something that is over.
 if(!bOpen&&(bRing||bWantOpen)&&Pack.NextLine()==0&&!Pack.StillWanted()&&Pack.Withdraw())
 {
  UE_LOG(LogS47StoryScene,Display,TEXT("S47: story scene %s withdrawn: its reason has gone before it was answered"),*Active);
  StopRing();bWantOpen=false;ReopenIn=0;Active.Reset();Channel.Reset();SpokenLine.Reset();
  return;
 }
 if(Channel==TEXT("phone"))return; // waits for the player at the telephone
 if(bOpen)return;
 if(ReopenIn>0){ReopenIn-=Dt;return;}
 if(bWantOpen&&M&&M->UI&&!M->ModalOpen())Show();
}

// After a step of the pack: the scene goes on (ring, or open), or it is over.
void FS47StoryScene::AfterStep()
{
 const FS47EveningScene* S=Pack.CurrentScene();
 if(!S){Finish();return;}
 Channel=S->Channel;
 if(bOpen){Show();return;}
 if(Channel==TEXT("phone"))StartRing();
 else bWantOpen=true;
}

bool FS47StoryScene::Continue()
{
 if(!Pack.CurrentLine())return false;
 if(Voice)Voice->Stop();
 Pack.Advance();
 // A scene whose last line has gone and has no choices completes on the next Advance (as the Story host does).
 if(const FS47EveningScene* S=Pack.CurrentScene();S&&!Pack.CurrentLine()&&S->Choices.IsEmpty())Pack.Advance();
 AfterStep();
 return true;
}

bool FS47StoryScene::Choose(const FString& ChoiceId)
{
 if(Pack.CurrentLine()||!Pack.Select(ChoiceId))return false;
 UE_LOG(LogS47StoryScene,Display,TEXT("S47: story scene %s: %s"),*Active,*ChoiceId);
 if(const FS47EveningScene* S=Pack.CurrentScene();S&&!Pack.CurrentLine()&&S->Choices.IsEmpty())Pack.Advance();
 AfterStep();
 return true;
}

void FS47StoryScene::Show()
{
 const FS47EveningScene* S=Pack.CurrentScene();
 bWantOpen=false;
 if(!S||!M||!M->UI)return;
 TArray<S47Kit::FW> Rows;
 const TWeakPtr<uint8> Guard=Life;
 if(const FS47EveningLine* L=Pack.CurrentLine())
 {
  Rows.Add(S47Kit::Sub(L->Speaker));
  Rows.Add(S47Kit::Text(L->Text));
  if(SpokenLine!=L->Id)
  {
   SpokenLine=L->Id;
   if(Voice)Voice->Stop();
   // Dale only, in his own voice; a line with no take stays text.
   if(L->Speaker==TEXT("DALE")&&(Channel==TEXT("phone")||Channel==TEXT("walkie"))&&IsValid(M->Audio))
   {
    if(!Voice||Voice->ChannelName()!=Channel)Voice=MakeShared<FS47DaleVoice>(M->Audio,Channel);
    const FS47DaleVoice::FPlan Plan=Voice->PlanLines({L->Text});
    if(Plan.Voiced()>0)Voice->Play(Plan);
   }
  }
  Rows.Add(S47Kit::Button(TEXT("Continue"),[this,Guard]{if(Guard.IsValid())Continue();},true));
 }
 else
 {
  Rows.Add(S47Kit::Sub(TEXT("Your reply")));
  for(const FS47EveningChoice* C:Pack.Choices())
  {
   const FString Id=C->Id;
   Rows.Add(S47Kit::Button(C->Label,[this,Guard,Id]{if(Guard.IsValid())Choose(Id);},Rows.Num()==1));
  }
 }
 bOpen=false; // Open closes the panel before (whose close callback is this host's own): the flag is set after
 M->UI->Open(S47Kit::Shell(M->UI,S47StorySceneLocal::TitleOf(Channel),S47Kit::Unread(S47Kit::Col(Rows)),820,true),[this,Guard]{if(Guard.IsValid())Close();});
 bOpen=true;
}

// The panel was closed (Escape, or a step that opens the next page): a scene still under way waits.
void FS47StoryScene::Close()
{
 if(!bOpen)return;
 bOpen=false;
 if(Voice)Voice->Stop();
 if(!Pack.CurrentScene())return;
 if(Channel==TEXT("phone")){if(M&&M->Web)M->Web->SetHandsetVisible(true);StartRing();}
 else{bWantOpen=true;ReopenIn=S47StorySceneLocal::ReopenAfter;}
}

void FS47StoryScene::Finish()
{
 const FString Was=Active;
 const bool bWasOpen=bOpen;
 Active.Reset();SpokenLine.Reset();bWantOpen=false;ReopenIn=0;
 StopRing();
 if(Voice)Voice->Stop();
 if(bWasOpen&&M&&M->UI){bOpen=false;M->UI->Close(true);}
 bOpen=false;
 if(Channel==TEXT("phone")&&M&&M->Web&&bWasOpen)
 {
  M->Web->SetHandsetVisible(true);
  if(M->Audio)M->Audio->Play(TEXT("handset_down"),.5f,M->Web->PropLocation(TEXT("phone")));
 }
 // The pack's flags that are set, and the scene's own, become story flags.
 TArray<FString> Set;
 if(const TSharedPtr<FJsonObject> Saved=Pack.Save();Saved.IsValid())
 {
  const TSharedPtr<FJsonObject>* Flags=nullptr;
  if(Saved->TryGetObjectField(TEXT("flags"),Flags)&&Flags&&Flags->IsValid())
   for(const auto& Pair:(*Flags)->Values){const FString Key(Pair.Key.ToView());bool bOn=false;if(Pair.Value.IsValid()&&Pair.Value->TryGetBool(bOn)&&bOn&&S47StorySceneLocal::ValidFlag(Key))Set.Add(Key);}
 }
 if(!Was.IsEmpty())Set.Add(TEXT("scene_")+Was.ToLower());
 for(const FString& F:Set)if(OnFlag)OnFlag(F);
 UE_LOG(LogS47StoryScene,Display,TEXT("S47: story scene %s over; story flags %s"),*Was,*FString::Join(Set,TEXT(", ")));
}

void FS47StoryScene::Stop()
{
 StopRing();
 if(Voice)Voice->Stop();
 if(bOpen&&M&&M->UI){bOpen=false;M->UI->Close(true);}
 bOpen=false;bWantOpen=false;ReopenIn=0;
 Active.Reset();SpokenLine.Reset();Channel.Reset();
 Pack.Reset();
}

S47J::FObj FS47StoryScene::Save() const
{
 const S47J::FObj O=S47J::New();
 O->SetObjectField(TEXT("pack"),Pack.Save());
 return O;
}

void FS47StoryScene::Load(const S47J::FObj& Saved)
{
 Stop();
 if(!bReady||!Saved.IsValid())return;
 const TSharedPtr<FJsonObject>* P=nullptr;
 if(!Saved->TryGetObjectField(TEXT("pack"),P)||!P||!P->IsValid())return;
 FString Error;
 if(!Pack.Load(*P,Error)){UE_LOG(LogS47StoryScene,Display,TEXT("S47: story scene: save not read (%s); none under way"),*Error);Pack.Reset();return;}
 if(const FS47EveningScene* S=Pack.CurrentScene()){Active=S->Id;Channel=S->Channel;AfterStep();}
}

bool FS47StoryScene::SelfTest(TArray<FString>& Lines)
{
 bool bOk=true;
 const auto Check=[&](bool bPass,const FString& What){Lines.Add(FString::Printf(TEXT("%s %s"),bPass?TEXT("PASS"):TEXT("FAIL"),*What));bOk&=bPass;};
 FS47StoryScene S(nullptr);
 Check(S.Ready()&&Known(TEXT("D01_HANDOVER"))&&!Known(TEXT("NO_SUCH_SCENE")),TEXT("STORY-5: the story scene host reads the pack; the lint knows its scenes"));
 TArray<FString> Got;
 S.OnFlag=[&Got](const FString& F){Got.Add(F);};
 FString Why;
 const bool bNeeds=!S.Begin(TEXT("D01_HANDOVER"),Why);
 const bool bWorld=S.Begin(TEXT("I11_SARO_ARRIVAL"),Why)&&!S.Busy()&&Got.Contains(TEXT("intro_arrived"))&&Got.Contains(TEXT("scene_i11_saro_arrival"));
 Check(bNeeds&&bWorld,TEXT("STORY-5: a scene waits for the pack's requirements; a world scene completes at once and its flags become story flags"));
 const bool bPhone=S.Begin(TEXT("D01_HANDOVER"),Why)&&S.Busy()&&S.bRing&&!S.Begin(TEXT("D01_HANDOVER"),Why);
 Check(bPhone,TEXT("STORY-5: the handover rings (phone), one scene at a time"));
 // Half way: save, and take it up again in another host.
 S.Continue();S.Continue();
 const S47J::FObj Mid=S.Save();
 FS47StoryScene T(nullptr);
 TArray<FString> GotT;
 T.OnFlag=[&GotT](const FString& F){GotT.Add(F);};
 T.Load(Mid);
 Check(T.Busy()&&T.SceneId()==TEXT("D01_HANDOVER")&&T.bRing,TEXT("STORY-5: a scene under way is saved and taken up again (it rings again)"));
 int32 Steps=0;
 while(T.Busy()&&Steps++<40)
 {
  if(T.Pack.CurrentLine()){T.Continue();continue;}
  const TArray<const FS47EveningChoice*> Cs=T.Pack.Choices();
  if(Cs.IsEmpty())break;
  T.Choose(Cs[0]->Id);
 }
 Check(!T.Busy()&&GotT.Contains(TEXT("handover_done"))&&GotT.Contains(TEXT("scene_d01_handover"))&&!T.bRing,TEXT("STORY-5: the handover plays to its end with a choice; handover_done and scene_d01_handover become story flags"));
 // Step 6: Dale's signal call rings while there is a signal; the signal goes before anyone picks up: it stops ringing and is taken back.
 T.SetContext({{TEXT("ctx_signal_present"),true}});
 const bool bRang=T.Begin(TEXT("D21_SIGNAL_PHONE"),Why)&&T.bRing;
 T.Tick(.1);
 const bool bStill=T.Busy()&&T.bRing;
 T.SetContext({{TEXT("ctx_signal_present"),false}});
 T.Tick(.1);
 Check(bRang&&bStill&&!T.Busy()&&!T.bRing&&!GotT.Contains(TEXT("dale_signal_call_done")),TEXT("STORY-6: a call whose reason goes before it is answered stops ringing and is taken back without its effects"));
 return bOk;
}

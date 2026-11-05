#include "S47CaptureSound.h"
#include "S47ReceiverTexture.h"
#include "S47WebAudio.h"
#include "HAL/PlatformTime.h"

namespace S47CaptureSound
{
 namespace
 {
  constexpr double Period=1.1879;
  constexpr int MaxFrames=8192;
  double Finite(const S47J::FObj& O,const TCHAR* Key,double Default=0){const double V=S47J::Num(O,Key,Default);return FMath::IsFinite(V)?V:Default;}
  float Pulse(double Time,double At,double Hz,double Length,double Level)
  {
   const double T=Time-At;if(T<0||T>=Length)return 0;
   const double Edge=FMath::Min3(1.,T/.001,(Length-T)/.001);
   return float(Level*Edge*(FMath::Sin(2*PI*Hz*T)>=0?1:-1));
  }
 }
 void FTrack::Begin(const FString& InId,bool InControl,const TArray<double>& ActualTicks)
 {
  *this=FTrack();Id=InId;Control=InControl;Duration=Control?8:18;if(!Control)Ticks=ActualTicks;
 }
 void FTrack::Interrupt(const FString& Reason){if(Enabled()&&Failure.IsEmpty()&&!Complete)Failure=Reason;}
 bool FTrack::Append(double From,double To,double Dt,double Rate,double Frequency,double Gain,double BW,double Quality)
 {
  if(!Eligible()||Complete)return false;
  const double Previous=Frames.IsEmpty()?0:Frames.Last().End;
  const bool FiniteInputs=FMath::IsFinite(From)&&FMath::IsFinite(To)&&FMath::IsFinite(Dt)&&FMath::IsFinite(Rate)&&FMath::IsFinite(Frequency)&&FMath::IsFinite(Gain)&&FMath::IsFinite(BW)&&FMath::IsFinite(Quality);
  if(!FiniteInputs||Dt<=0||Dt>.100001||FMath::Abs(Rate-1)>1e-6||FMath::Abs(Previous-From)>1e-6||To<=From||To>Duration+1e-6||FMath::Abs((To-From)-FMath::Min(Dt,Duration-From))>1e-6||Frames.Num()>=MaxFrames||Frequency<1419.5||Frequency>1420.7||Gain<0||Gain>100||BW<0||BW>100||Quality<0||Quality>1)
  {Interrupt(TEXT("Continuous receiver capture was interrupted; retained measurement remains available."));return false;}
  FFrame F;F.Start=From;F.End=To;F.Frequency=Frequency;F.Gain=FMath::Clamp(Gain,0.,100.);F.Bandwidth=FMath::Clamp(BW,0.,100.);F.Quality=FMath::Clamp(Quality,0.,1.);Frames.Add(F);return true;
 }
 float FTrack::Sample(double At)const
 {
  if(At<0||At>=Duration||Frames.IsEmpty())return 0;
  int Lo=0,Hi=Frames.Num();while(Lo<Hi){const int Mid=(Lo+Hi)/2;if(Frames[Mid].End<=At)Lo=Mid+1;else Hi=Mid;}
  if(Lo>=Frames.Num()||At<Frames[Lo].Start)return 0;
  const auto& F=Frames[Lo];const auto& Prev=Frames[FMath::Max(0,Lo-1)];
  const double Blend=FMath::Clamp((At-F.Start)/.008,0.,1.);
  const double Gain=FMath::Lerp(Prev.Gain,F.Gain,Blend)/100.,BW=FMath::Lerp(Prev.Bandwidth,F.Bandwidth,Blend)/100.,Q=FMath::Lerp(Prev.Quality,F.Quality,Blend);
  // v3 deliberately renders both live and retained background via this same
  // stateless v2 texture, not the old monitor's stateful filter reconstruction.
  float V=.42f*S47ReceiverTexture::Sample(S47ReceiverTexture::Seed(TEXT("freeform-source-1")),At,float(Q),float(Gain),float(BW),2);
  for(double P=Period;P<Duration;P+=Period)V+=Pulse(At,P,820,.012,.16);
  if(!Control)for(double P:Ticks)V+=Pulse(At,P,1240,.008,.1);
  return V*float(FMath::Min3(1.,At/.008,(Duration-At)/.008));
 }
 S47J::FObj FTrack::Save()const
 {
  if(!Enabled())return nullptr;auto O=S47J::New();S47J::Set(O,TEXT("v"),3);S47J::Set(O,TEXT("id"),Id);S47J::Set(O,TEXT("control"),Control);S47J::Set(O,TEXT("duration"),Duration);S47J::Set(O,TEXT("failure"),Failure);S47J::Set(O,TEXT("complete"),Complete);S47J::Set(O,TEXT("ticks"),Ticks);
  TArray<TSharedPtr<FJsonValue>> A;for(const auto& F:Frames){auto R=S47J::New();S47J::Set(R,TEXT("from"),F.Start);S47J::Set(R,TEXT("to"),F.End);S47J::Set(R,TEXT("frequency"),F.Frequency);S47J::Set(R,TEXT("gain"),F.Gain);S47J::Set(R,TEXT("bandwidth"),F.Bandwidth);S47J::Set(R,TEXT("quality"),F.Quality);A.Add(MakeShared<FJsonValueObject>(R));}O->SetArrayField(TEXT("frames"),A);return O;
 }
 void FTrack::Load(const S47J::FObj& O)
 {
  *this=FTrack();if(!O||S47J::Int(O,TEXT("v"))!=3)return;
  Begin(S47J::Str(O,TEXT("id")),S47J::Bool(O,TEXT("control")),S47J::Nums(O,TEXT("ticks")));if(!Enabled())return;
  if(Finite(O,TEXT("duration"),-1)!=Duration||Id.Len()>256||Ticks.Num()>128){Interrupt(TEXT("Retained capture data unavailable."));return;}
  for(double T:Ticks)if(!FMath::IsFinite(T)||T<0||T>Duration){Interrupt(TEXT("Retained capture data unavailable."));return;}
  for(const auto& Value:S47J::Arr(O,TEXT("frames")))
  {
   const S47J::FObj* R=nullptr;if(!Value.IsValid()||!Value->TryGetObject(R)||!R){Interrupt(TEXT("Retained capture data unavailable."));break;}
   const double From=Finite(*R,TEXT("from"),-1),To=Finite(*R,TEXT("to"),-1);
   if(!Append(From,To,To-From,1,Finite(*R,TEXT("frequency")),Finite(*R,TEXT("gain"),-1),Finite(*R,TEXT("bandwidth"),-1),Finite(*R,TEXT("quality"),-1)))break;
  }
  const FString SavedFailure=S47J::Str(O,TEXT("failure"));if(!SavedFailure.IsEmpty())Failure=SavedFailure;
  Complete=Eligible()&&S47J::Bool(O,TEXT("complete"))&&!Frames.IsEmpty()&&FMath::IsNearlyEqual(Frames.Last().End,Duration,1e-6);
  if(!Complete&&Failure.IsEmpty())Interrupt(TEXT("Recording resumed from a save; retained measurements use compatible reference sound."));
 }
 TSharedPtr<FS47AudioBuffer> Buffer(const FTrack& T,const TSharedRef<FS47AudioContext>& Ctx,int64 First,int64 Last)
 {
  if(First<0||Last<=First||Last>FMath::RoundToInt64(T.Duration*Ctx->SampleRate))return nullptr;
  auto B=Ctx->CreateBuffer(1,int32(Last-First),Ctx->SampleRate);float* P=B->GetChannelData(0);
  for(int64 I=First;I<Last;++I)P[I-First]=T.Sample(double(I)/Ctx->SampleRate);return B;
 }
}

namespace
{
 // First integer sample whose actual Sample(I/Rate) time is >= Time.
 // ceil(Time*Rate) alone can include I/Rate==Time in the previous block
 // because multiplication and division round differently (e.g.8.05 at8k).
 int64 CaptureSampleBoundary(double Time,double Rate)
 {
  int64 I=FMath::CeilToInt64(Time*Rate);
  while(I>0&&double(I-1)/Rate>=Time)--I;
  while(double(I)/Rate<Time)++I;
  return I;
 }
}

// A private mute gate lets the recorder continue while the physical receiver is
// out of earshot. Graph mutation and buffer ownership stay under Context::Lock.
class FS47CapturePlayer::FGate : public FS47GainNode
{
public:explicit FGate(const TSharedRef<FS47AudioContext>& C):FS47GainNode(C){}
};
FS47CapturePlayer::FS47CapturePlayer(const TSharedRef<FS47AudioContext>& C,const TSharedPtr<FS47AudioNode>& D):Ctx(C),Destination(D){}
FS47CapturePlayer::~FS47CapturePlayer(){Stop();}
void FS47CapturePlayer::Stop()
{
 FS47AudioContext::FBatch Batch(*Ctx);for(auto& B:Sources){B.Source->Stop();B.Source->Disconnect();}Sources.Empty();if(Gate)Gate->Disconnect();Gate.Reset();Id.Empty();BaseFrame=NextFrame=EndFrame=DoneSamples=ReplayOffset=0;Replaying=false;
}
void FS47CapturePlayer::Audible(bool Enabled){FS47AudioContext::FBatch Batch(*Ctx);if(Gate)Gate->Gain.SetValue(Enabled?1:0);}
int64 FS47CapturePlayer::RenderedSamples()const
{FS47AudioContext::FBatch Batch(*Ctx);int64 Count=DoneSamples;for(const auto& B:Sources)if(B.Source->Ended())Count+=B.Count;return Count;}
bool FS47CapturePlayer::Aligned(double Cursor)const
{
 if(!Replaying)return true;
 FS47AudioContext::FBatch Batch(*Ctx);const double Actual=double(ReplayOffset+FMath::Max<int64>(0,FMath::RoundToInt64(Ctx->CurrentTime()*Ctx->SampleRate)-BaseFrame))/Ctx->SampleRate;
 const bool Good=FMath::IsFinite(Cursor)&&FMath::Abs(Actual-Cursor)<=.05+double(S47Quantum)/Ctx->SampleRate;
 if(!Good)UE_LOG(LogTemp,Display,TEXT("S47 capture playback drift cursor=%.9f audio=%.9f rate=%.0f"),Cursor,Actual,Ctx->SampleRate);return Good;
}
bool FS47CapturePlayer::Pending()const{return !Id.IsEmpty()&&Ctx->CurrentTime()*Ctx->SampleRate<EndFrame;}
bool FS47CapturePlayer::Queue(const S47CaptureSound::FTrack& T)
{
 if(!T.Eligible()||T.Frames.IsEmpty()||Ctx->State()!=TEXT("running")||!Destination)return false;
 const auto& F=T.Frames.Last();const int64 First=CaptureSampleBoundary(F.Start,Ctx->SampleRate),Last=CaptureSampleBoundary(F.End,Ctx->SampleRate);
 if(Last<=First)return false;auto Buffer=S47CaptureSound::Buffer(T,Ctx,First,Last);if(!Buffer)return false;
 FS47AudioContext::FBatch Batch(*Ctx);const int64 Now=FMath::RoundToInt64(Ctx->CurrentTime()*Ctx->SampleRate),Lead=FMath::CeilToInt64(.12*Ctx->SampleRate);
 if(Id!=T.Id)
 {
  Stop();if(First!=0)return false;Id=T.Id;BaseFrame=Now+Lead;Gate=MakeShared<FGate>(Ctx);Gate->Gain.SetValue(0);Gate->Connect(Destination);
 }
 // Keep the existing 120-ms scheduling lead and late deadline. The capture
 // cursor may not outrun elapsed rendered audio by more than that same lead.
 // State()==running alone is insufficient: a device can stop pulling Render.
 const int64 RenderedSinceStart=Now-(BaseFrame-Lead);
 if(Replaying||First!=NextFrame||Now>=BaseFrame+First||First-RenderedSinceStart>Lead){Stop();return false;}
 Sources.RemoveAll([&](const FBlock& B){if(B.Source->Ended()){DoneSamples+=B.Count;return true;}return false;});
 auto Source=Ctx->CreateBufferSource();Source->Buffer=Buffer;Source->Connect(Gate);
 // ScheduledSource rounds When upward: choose a point strictly inside the
 // intended integer sample, avoiding floating division rounding to next sample.
 Source->Start((double(BaseFrame+First)-.25)/Ctx->SampleRate);
 Sources.Add({Source,BaseFrame+Last,Last-First});NextFrame=Last;EndFrame=BaseFrame+Last;return true;
}
bool FS47CapturePlayer::Ready(const S47CaptureSound::FTrack& T)const
{return T.Playable()&&CachedId==T.Id&&CachedBuffer.IsValid();}
bool FS47CapturePlayer::Prepare(const S47CaptureSound::FTrack& T)
{
 if(!T.Playable()||!Destination)return false;
 Stop();if(CachedId!=T.Id||!CachedBuffer){const double Began=FPlatformTime::Seconds();CachedBuffer=S47CaptureSound::Buffer(T,Ctx,0,FMath::RoundToInt64(T.Duration*Ctx->SampleRate));CachedId=T.Id;UE_LOG(LogTemp,Display,TEXT("S47 capture prepare id=%s seconds=%.6f"),*T.Id,FPlatformTime::Seconds()-Began);}
 return Ready(T);
}
bool FS47CapturePlayer::Play(const S47CaptureSound::FTrack& T,double Cursor)
{
 if(!T.Playable()||Cursor<0||Cursor>T.Duration||Ctx->State()!=TEXT("running")||!Destination)return false;
 if(Replaying&&Id==T.Id){if(Aligned(Cursor))return true;Stop();return false;}
 if(Cursor>=T.Duration||!Ready(T))return false;Stop();const auto Buffer=CachedBuffer;
 FS47AudioContext::FBatch Batch(*Ctx);const int64 Now=FMath::RoundToInt64(Ctx->CurrentTime()*Ctx->SampleRate),Offset=FMath::RoundToInt64(Cursor*Ctx->SampleRate);
 Id=T.Id;Replaying=true;ReplayOffset=Offset;BaseFrame=Now+S47Quantum;EndFrame=BaseFrame+Buffer->Length()-Offset;
 Gate=MakeShared<FGate>(Ctx);Gate->Gain.SetValue(1);Gate->Connect(Destination);
 auto Source=Ctx->CreateBufferSource();Source->Buffer=Buffer;Source->Connect(Gate);Source->Start((double(BaseFrame)-.25)/Ctx->SampleRate,double(Offset)/Ctx->SampleRate);NextFrame=Buffer->Length()-Offset;Sources.Add({Source,EndFrame,NextFrame});return true;
}

bool S47CaptureSound::SelfTest(TArray<FString>& Lines)
{
 bool Good=true;auto Check=[&](bool B,const TCHAR* Why){Good&=B;Lines.Add(FString(B?TEXT("PASS "):TEXT("FAIL "))+TEXT("capture sound: ")+Why);};
 for(const int TestRate:{8000,48000})
 {
 bool Boundaries=true;
 for(int I=1;I<=360;++I){const double End=I*.05;const int64 N=CaptureSampleBoundary(End,TestRate);Boundaries&=double(N)/TestRate>=End&&(N==0||double(N-1)/TestRate<End);}
 Check(Boundaries,TEXT("integer queue boundaries agree with renderer time comparison; no future-frame sample enters live prefix"));
 auto Live=FS47AudioContext::Make(TestRate);FS47CapturePlayer Player(Live,Live->Destination);
 FTrack Track;Track.Begin(TEXT("test-original"),false,{9.2,12.4});TArray<float> Actual;bool Queued=true;
 for(int I=0;I<360;++I)
 {
  const double From=I*.05,To=(I+1)*.05;
  Queued&=Track.Append(From,To,.05,1,1420.11,I<120?40:75,I<200?20:70,I<240?.9:.65)&&Player.Queue(Track);Player.Audible(true);
  const int Offset=Actual.AddZeroed(TestRate/20);Live->Render(Actual.GetData()+Offset,TestRate/20,1);Live->Tick(.05);
 }
 const int64 Start=Player.ScheduledStart();const int Tail=Actual.AddZeroed(TestRate/4);Live->Render(Actual.GetData()+Tail,TestRate/4,1);Track.Complete=Queued;
 Check(Queued&&Track.Playable()&&Start>0,TEXT("actual finite source queue accepts full active capture and drains final block"));
 auto Expected=Buffer(Track,Live,0,18*TestRate);double MaxError=0,Energy=0;
 int Worst=-1,FirstDifference=-1;
 if(Start+18*TestRate<=Actual.Num())for(int I=0;I<18*TestRate;++I){const double Error=FMath::Abs(Actual[int(Start)+I]-Expected->GetChannelData(0)[I]);if(Error>MaxError){MaxError=Error;Worst=I;}if(Error>1e-5&&FirstDifference<0)FirstDifference=I;Energy+=FMath::Square(double(Actual[int(Start)+I]));}else MaxError=1;
 UE_LOG(LogTemp,Display,TEXT("S47 capture PCM audit rate=%d start=%lld maxError=%.9g energy=%.9g first=%d worst=%d actual=%.9g expected=%.9g"),TestRate,Start,MaxError,Energy,FirstDifference,Worst,Worst>=0?Actual[int(Start)+Worst]:0,Worst>=0?Expected->GetChannelData(0)[Worst]:0);
 Check(MaxError<1e-5&&Energy>1,TEXT("audio-context rendered queued live PCM matches retained recipe across actual setting changes and events"));
 FTrack Reload;Reload.Load(S47J::Read(S47J::Write(Track.Save())));Check(Reload.Playable()&&FJsonValue::CompareEqual(FJsonValueObject(Reload.Save()),FJsonValueObject(Track.Save())),TEXT("completed recipe survives serialized Continue exactly; no current receiver state consulted"));
 auto Replay=FS47AudioContext::Make(TestRate);FS47CapturePlayer Deck(Replay,Replay->Destination);Check(!Deck.Play(Reload,7.25)&&Deck.Prepare(Reload)&&Deck.Ready(Reload)&&!Deck.Pending()&&Deck.QueuedSamples()==0,TEXT("explicit preparation produces ready PCM without starting transport"));
 TArray<float> Loading;Loading.SetNumZeroed(TestRate/2);Replay->Render(Loading.GetData(),Loading.Num(),1);double LoadingEnergy=0;for(float V:Loading)LoadingEnergy+=FMath::Abs(V);
 Check(LoadingEnergy==0&&Deck.RenderedSamples()==0&&Deck.Ready(Reload),TEXT("audio time during tape preparation grants no playback or source completion"));
 Check(Deck.Play(Reload,7.25),TEXT("retained playback starts actual buffer source at explicit seek offset"));
 const int64 ReplayOrigin=TestRate/2;TArray<float> Played;Played.SetNumZeroed(12*TestRate);Replay->Render(Played.GetData(),Played.Num(),1);const int Begin=int(Deck.ScheduledStart()-ReplayOrigin);MaxError=0;
 for(int I=0;I<10*TestRate;++I)MaxError=FMath::Max(MaxError,double(FMath::Abs(Played[Begin+I]-Expected->GetChannelData(0)[int(7.25*TestRate)+I])));
 Check(MaxError<1e-5,TEXT("actual source seek playback preserves phase and captured settings rather than restarting at zero"));
 Deck.Stop();TArray<float> Quiet;Quiet.SetNumZeroed(512);Replay->Render(Quiet.GetData(),512,1);double Q=0;for(int I=128;I<512;++I)Q+=FMath::Abs(Quiet[I]);Check(Q<1e-6,TEXT("pause/seek stop removes pending source after current audio quantum"));
 Check(Deck.Play(Reload,2),TEXT("play after pause creates a fresh offset transport"));
 TArray<float> Stall;Stall.SetNumZeroed(TestRate/2);Replay->Render(Stall.GetData(),Stall.Num(),1);
 Check(!Deck.Aligned(2)&&!Deck.Play(Reload,2)&&!Deck.Pending(),TEXT("actual audio advancing without game cursor rejects drift and stops before further listening coverage"));
 Check(Deck.Play(Reload,2)&&Deck.Aligned(2),TEXT("explicit play after drift starts fresh phase at unchanged cursor"));Deck.Stop();
 FTrack Control;Control.Begin(TEXT("test-control"),true,{});for(int I=0;I<160;++I)Control.Append(I*.05,(I+1)*.05,.05,1,1420.11,20,10,1);Control.Complete=true;
 Check(Control.Playable()&&Control.Ticks.IsEmpty()&&Control.Sample(4)!=Track.Sample(4)&&Track.Ticks.Num()==2,TEXT("own eight-second internal-reference settings and source events never rewrite original"));
 // Uneven frame/sample boundaries and a clamped final step exercise the
 // queue boundary rule that fixed 50-ms frames alone cannot verify.
 const int ControlRate=48000;auto ControlCtx=FS47AudioContext::Make(ControlRate);FS47CapturePlayer ControlDeck(ControlCtx,ControlCtx->Destination);
 FTrack Uneven;Uneven.Begin(TEXT("uneven-control"),true,{});TArray<float> ControlPCM;bool ControlOK=true;double Previous=0;int64 Rendered=0;
 for(int I=1;Previous<8;++I)
 {
  const double End=FMath::Min(8.,double(I)/73.1);ControlOK&=Uneven.Append(Previous,End,1./73.1,1,1420.11,20,10,1)&&ControlDeck.Queue(Uneven);ControlDeck.Audible(true);
  const int64 Target=FMath::FloorToInt64(End*ControlRate);const int Count=int(Target-Rendered);const int Offset=ControlPCM.AddZeroed(Count);ControlCtx->Render(ControlPCM.GetData()+Offset,Count,1);Rendered=Target;Previous=End;
 }
 const int64 ControlStart=ControlDeck.ScheduledStart();const int ControlTail=ControlPCM.AddZeroed(8192);ControlCtx->Render(ControlPCM.GetData()+ControlTail,8192,1);Uneven.Complete=ControlOK;
 auto ControlExpected=Buffer(Uneven,ControlCtx,0,8*ControlRate);double ControlError=0;
 if(ControlStart+8*ControlRate<=ControlPCM.Num())for(int I=0;I<8*ControlRate;++I)ControlError=FMath::Max(ControlError,double(FMath::Abs(ControlPCM[int(ControlStart)+I]-ControlExpected->GetChannelData(0)[I])));else ControlError=1;
 Check(ControlOK&&Uneven.Playable()&&ControlError<1e-5&&ControlDeck.RenderedSamples()==8*ControlRate,TEXT("actual own control PCM survives uneven frame boundaries, final clamp and complete drain"));
 auto ControlReplay=FS47AudioContext::Make(ControlRate);FS47CapturePlayer ControlSeek(ControlReplay,ControlReplay->Destination);Check(ControlSeek.Prepare(Uneven)&&ControlSeek.Play(Uneven,1.234),TEXT("48k own-reference actual source seeks to non-quantum offset"));
 TArray<float> ControlPlayed;ControlPlayed.SetNumZeroed(2*ControlRate+S47Quantum);ControlReplay->Render(ControlPlayed.GetData(),ControlPlayed.Num(),1);double SeekError=0;
 const int SeekStart=int(ControlSeek.ScheduledStart()),SeekOffset=FMath::RoundToInt(1.234*ControlRate);
 for(int I=0;I<ControlRate;++I)SeekError=FMath::Max(SeekError,double(FMath::Abs(ControlPlayed[SeekStart+I]-ControlExpected->GetChannelData(0)[SeekOffset+I])));
 Check(SeekError<1e-5,TEXT("48k actual control playback retains exact saved phase/settings after seek"));
 FTrack Partial;Partial.Begin(TEXT("partial"),false,{});Partial.Append(0,.05,.05,1,1420.11,50,50,1);FTrack Continued;Continued.Load(Partial.Save());Check(!Continued.Playable()&&!Continued.Failure.IsEmpty(),TEXT("mid-capture Continue explicitly falls back without fabricating audio continuity"));
 Check(!Partial.Append(.05,.075,.05,.5,1420.11,50,50,1)&&!Partial.Failure.IsEmpty(),TEXT("partial array rate rejects phase capture without changing acquisition itself"));
 FTrack Gap;Gap.Begin(TEXT("gap"),false,{});Gap.Append(0,.05,.05,1,1420.11,50,50,1);Gap.Interrupt(TEXT("held"));Check(!Gap.Append(.05,.1,.05,1,1420.11,50,50,1)&&!Gap.Playable(),TEXT("explicit held/power/tuning gap cannot be joined into a continuous capture"));
 // No Render calls: Tick and a running context do not establish audio progress.
 // The same guard is exercised at both rates already used by the PCM audit.
 auto Frozen=FS47AudioContext::Make(TestRate);FS47CapturePlayer Unrendered(Frozen,Frozen->Destination);FTrack FrozenTrack;FrozenTrack.Begin(TEXT("running-without-render"),false,{});
 bool FrozenRejected=false;double AcceptedEnd=0;
 for(int I=0;I<360;++I)
 {
  const double From=I*.05,To=(I+1)*.05;
  if(!FrozenTrack.Append(From,To,.05,1,1420.11,50,50,1)||!Unrendered.Queue(FrozenTrack)){FrozenTrack.Interrupt(TEXT("audio clock did not advance"));FrozenRejected=true;break;}
  AcceptedEnd=To;Frozen->Tick(.05);
 }
 Check(Frozen->State()==TEXT("running")&&Frozen->CurrentTime()==0&&FrozenRejected&&AcceptedEnd>0&&AcceptedEnd<=.2&&!FrozenTrack.Playable()&&!Unrendered.Pending()&&Unrendered.QueuedSamples()==0&&Unrendered.RenderedSamples()==0,TEXT("running context without rendered clock cannot accept full capture; bounded startup lead is allowed then queue is stopped"));
 auto Late=FS47AudioContext::Make(8000);FS47CapturePlayer Behind(Late,Late->Destination);FTrack LateTrack;LateTrack.Begin(TEXT("late"),false,{});LateTrack.Append(0,.05,.05,1,1420.11,50,50,1);Check(Behind.Queue(LateTrack),TEXT("deadline fixture starts with actual queued block"));
 TArray<float> Delay;Delay.SetNumZeroed(4000);Late->Render(Delay.GetData(),4000,1);LateTrack.Append(.05,.1,.05,1,1420.11,50,50,1);Check(!Behind.Queue(LateTrack)&&!Behind.Pending(),TEXT("late scheduling refuses overlap and stops all pending blocks before compatibility fallback"));
 }
 return Good;
}

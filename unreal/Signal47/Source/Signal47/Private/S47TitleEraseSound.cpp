#include "S47TitleEraseSound.h"
#include "S47Audio.h"
#include "Misc/FileHelper.h"

namespace
{
 constexpr int32 Rate=48000;
 double Ease(double X) { X=FMath::Clamp(X,0.,1.); return X*X*(3.-2.*X); }
 struct FBreak { double At,Width,Depth; };
 const FBreak Breaks[]={{.23,.027,.75},{.47,.045,.9},{.73,.019,.65},{.91,.038,.85},{1.12,.024,.8}};

 TSharedPtr<FS47AudioBuffer> MakeErase(bool Calm)
 {
  auto B=MakeShared<FS47AudioBuffer>(1,FMath::RoundToInt(Rate*FS47TitleEraseSound::Duration),Rate);
  FRandomStream R(471128); double Carrier=0,Release=0,Low=0,Slow=0;
  for(int32 I=0;I<B->Length();++I)
  {
   const double T=double(I)/Rate;
   const double N=R.FRandRange(-1.f,1.f);
   Low+=(N-Low)*.20; Slow+=(N-Slow)*.014;
   const double Band=Low-Slow;
   const double Slip=FMath::Sin(T*2.*PI*8.1)*.006+FMath::Sin(T*2.*PI*4.7+.8)*.004;
   Carrier+=2.*PI*(181.-T*11.)*(1.+Slip)/Rate;
   double Drop=1,Edges=0;
   for(const FBreak& G:Breaks)
   {
    const double Local=T-G.At;
    const double W=Ease(Local/.006)*Ease((G.Width-Local)/.009);
    Drop*=1.-W*G.Depth*(Calm?.43:1.);
    // A small gated electrical grain at the irregular breaks, with shaped
    // edges. No impulse train or abrupt sample discontinuities.
    Edges+=W*(Calm?.32:1.);
   }
   const double BodyGate=Ease(T/.055)*Ease((1.39-T)/.23);
   const double Tone=.66*FMath::Sin(Carrier)+.19*FMath::Sin(Carrier*1.017)+.07*FMath::Sin(Carrier*2.73);
   const double Body=(Tone*.032+Band*.038)*BodyGate*Drop;
   const double Grain=Band*Edges*.047*BodyGate;
   const double U=FMath::Clamp((T-1.19)/.46,0.,1.);
   Release+=2.*PI*(205.-163.*Ease(U))/Rate;
   const double ReleaseGate=Ease((T-1.19)/.07)*Ease((FS47TitleEraseSound::Duration-T)/.23);
   const double Suck=(Band*.031+FMath::Sin(Release)*.026)*ReleaseGate;
   const double Outer=Ease((FS47TitleEraseSound::Duration-T)/.035);
   B->GetChannelData(0)[I]=float((Body+Grain+Suck)*Outer*(Calm?.58:1.));
  }
  return B;
 }
 US47Audio* Offline()
 {
  auto* A=NewObject<US47Audio>(GetTransientPackage()); A->AddToRoot(); A->bReady=true;
  A->Ctx=FS47AudioContext::Make(Rate);
  A->Master=A->Ctx->CreateGain(); A->Master->Gain.SetValue(.8); A->Master->Connect(A->Ctx->Destination);
  A->SfxBus=A->Ctx->CreateGain(); A->SfxBus->Connect(A->Master); return A;
 }
 void Close(US47Audio* A) { A->Ctx->Close(); A->RemoveFromRoot(); }
 bool Wave(const FString& Path,const TArray<float>& Samples)
 {
  TArray<uint8> Data;
  const auto Put16=[&](uint16 V){Data.Add(uint8(V));Data.Add(uint8(V>>8));};
  const auto Put32=[&](uint32 V){Data.Add(uint8(V));Data.Add(uint8(V>>8));Data.Add(uint8(V>>16));Data.Add(uint8(V>>24));};
  const auto Tag=[&](const ANSICHAR* T){for(int32 I=0;I<4;++I)Data.Add(uint8(T[I]));};
  const uint32 Bytes=Samples.Num()*2;
  Tag("RIFF");Put32(36+Bytes);Tag("WAVE");Tag("fmt ");Put32(16);Put16(1);Put16(2);
  Put32(Rate);Put32(Rate*4);Put16(4);Put16(16);Tag("data");Put32(Bytes);
  for(float V:Samples)Put16(uint16(int16(FMath::RoundToInt(FMath::Clamp(FMath::IsFinite(V)?V:0.f,-1.f,1.f)*32767.f))));
  return FFileHelper::SaveArrayToFile(Data,*Path);
 }
}

FS47TitleEraseSound::~FS47TitleEraseSound() { Stop(); }

bool FS47TitleEraseSound::Start(US47Audio* A,bool Calm)
{
 if(bStarted || !A || !A->bReady || !A->Ctx || !A->SfxBus || A->Ctx->State()!=TEXT("running"))return false;
 FS47AudioContext::FBatch Batch(*A->Ctx);
 Source=A->Ctx->CreateBufferSource(); Source->Buffer=MakeErase(Calm);
 Gain=A->Ctx->CreateGain(); Gain->Gain.SetValue(1);
 Source->Connect(Gain); Gain->Connect(A->SfxBus); Source->Start(A->Ctx->CurrentTime());
 bStarted=true; return true;
}

void FS47TitleEraseSound::Stop()
{
 if(Source && !Source->Ended())
 {
  FS47AudioContext::FBatch Batch(*Source->Ctx); const double T=Source->Ctx->CurrentTime();
  Gain->Gain.CancelAndHoldAtTime(T); Gain->Gain.LinearRampToValueAtTime(0,T+.035); Source->Stop(T+.036);
 }
 Source.Reset(); Gain.Reset();
}

bool FS47TitleEraseSound::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool Pass,const TCHAR* Text)
 {OK&=Pass;Lines.Add(FString::Printf(TEXT("%s title erasure audio: %s"),Pass?TEXT("PASS"):TEXT("FAIL"),Text));};
 const auto Normal=MakeErase(false), Calm=MakeErase(true);
 double EN=0,EC=0,Peak=0,Jump=0; bool Finite=true;
 for(int32 I=0;I<Normal->Length();++I)
 {
  const double N=Normal->GetChannelData(0)[I], C=Calm->GetChannelData(0)[I];
  Finite&=FMath::IsFinite(N)&&FMath::IsFinite(C); EN+=N*N; EC+=C*C; Peak=FMath::Max(Peak,FMath::Abs(N));
  if(I)Jump=FMath::Max(Jump,FMath::Abs(N-Normal->GetChannelData(0)[I-1]));
 }
 Check(Finite && Peak>.015 && Peak<.1 && Jump<.065,TEXT("bounded finite signal has no large single-sample pops"));
 Check(EC<EN*.55,TEXT("calm variant is measurably softer with the same visual duration"));
 Check(FMath::Abs(Normal->Duration()-Duration)<.0001 && FMath::Abs(Normal->GetChannelData(0)[0])<1e-6 &&
  FMath::Abs(Normal->GetChannelData(0)[Normal->Length()-1])<1e-6,TEXT("1.65-second cue begins and ends at silence"));
 auto* A=Offline(); FS47TitleEraseSound Sound;
 TArray<float> Render; Render.SetNumZeroed(Rate/5*2);
 Check(Sound.Start(A,false) && !Sound.Start(A,false),TEXT("repeated title callbacks cannot duplicate the sound"));
 A->Ctx->Render(Render.GetData(),Rate/5,2); A->Ctx->Tick(.2);
 Peak=0;for(float V:Render)Peak=FMath::Max(Peak,FMath::Abs(double(V)));
 Check(Peak>.001 && Peak<.1,TEXT("actual private graph renders the title cue at a restrained level"));
 Sound.Stop(); A->Ctx->Render(Render.GetData(),Rate/5,2); A->Ctx->Tick(.2);
 Peak=0;for(int32 I=Rate/10*2;I<Render.Num();++I)Peak=FMath::Max(Peak,FMath::Abs(double(Render[I])));
 Check(Peak<1e-6 && !Sound.Start(A,false),TEXT("leaving the title stops its source and does not re-arm it"));
 FS47TitleEraseSound Muted;A->SfxBus->Gain.SetValue(0);Muted.Start(A,false);
 A->Ctx->Render(Render.GetData(),Rate/5,2);A->Ctx->Tick(.2);
 Peak=0;for(int32 I=256;I<Render.Num();++I)Peak=FMath::Max(Peak,FMath::Abs(double(Render[I])));
 Check(Peak<1e-6,TEXT("existing SFX/master path really mutes the rendered cue"));
 Muted.Stop();Close(A);return OK;
}

bool FS47TitleEraseSound::RenderTour(const FString& Path)
{
 auto* A=Offline(); FS47TitleEraseSound Normal,Calm;
 constexpr int32 Frames=Rate*4,Chunk=800;
 TArray<float> Render;Render.SetNumZeroed(Frames*2);
 for(int32 F=0;F<Frames;F+=Chunk)
 {
  if(F==0)Normal.Start(A,false);
  if(F==Rate*2)Calm.Start(A,true);
  A->Ctx->Render(Render.GetData()+F*2,Chunk,2);A->Ctx->Tick(double(Chunk)/Rate);
 }
 Normal.Stop();Calm.Stop();Close(A);return Wave(Path,Render);
}

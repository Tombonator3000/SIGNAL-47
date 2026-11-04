#include "S47ReceiverTexture.h"
#include "S47WebAudio.h"

namespace
{
 constexpr double Tau=6.28318530717958647692;
 uint32 Mix(uint32 X)
 {
  X^=X>>16;X*=0x7feb352du;X^=X>>15;X*=0x846ca68bu;return X^(X>>16);
 }
 double Unit(uint32 X){return double(Mix(X)&0x00ffffffu)/16777215.;}
 double Phase(uint32 S,uint32 Salt){return Tau*Unit(S^Salt);}
 double Normal(float X,double Fallback=0.){return FMath::IsFinite(X)?FMath::Clamp(double(X),0.,1.):Fallback;}
 double Position(double T)
 {
  if(!FMath::IsFinite(T))return 0;
  const double P=FMath::Fmod(T,S47ReceiverTexture::LoopSeconds)/S47ReceiverTexture::LoopSeconds;
  return P<0?P+1:P;
 }
 // A periodic, smooth noise lattice. No state, allocation, random global or filter
 // warm-up, so a tape seek and a live receiver resolve exactly the same texture.
 double Noise(uint32 S,double U,uint32 Cells)
 {
  const double X=U*Cells;const uint32 I=uint32(X);const double F=X-I;
  const double A=Unit(S^(I%Cells)*0x9e3779b9u)*2-1;
  const double B=Unit(S^((I+1)%Cells)*0x9e3779b9u)*2-1;
  const double Blend=F*F*(3-2*F);
  return A+(B-A)*Blend;
 }
 float Layer(uint32 S,double U,S47ReceiverTexture::ELayer Which,float Bandwidth)
 {
  const double BW=Normal(Bandwidth,1);
  const double Low=Noise(S^0xc461u,U,3072);  // 256 noise knots/s: a soft radio body.
  const double Mid=Noise(S^0x2c87u,U,18432); // 1536 knots/s: grain, not a tonal beep.
  const double Air=Noise(S^0x853du,U,49152); // 4096 knots/s: open-band hiss.
  const double Band=.42*Low+(.28+.12*BW)*Mid+.18*BW*Air;
  if(Which==S47ReceiverTexture::ELayer::Static)
  {
   // Continuous irregular gain variation, not gated events or countable strikes.
   const double Drift=.84+.16*Noise(S^0x362du,U,19);
   const double Grain=Noise(S^0x1721u,U,9973)*Noise(S^0x7043u,U,43);
   return float(.82*Band*Drift+.12*BW*Grain);
  }
  const double P1=Phase(S,0x5741u),P2=Phase(S,0x9143u);
  const double Motion=Noise(S^0x9517u,U,11);
  const double Detail=Noise(S^0x3781u,U,29);
  // Integer cycles over the loop keep phase and derivative continuous. A source's
  // stable seed shifts its slow contours and modest spectral centre, never facts.
  const double F1=5640+int32(Mix(S)%211u); // ~470-487Hz with continuous FM warble.
  const double F2=9276+int32(Mix(S^0xb165u)%379u);
  const double W1=FMath::Sin(Tau*F1*U+24*FMath::Sin(Tau*3*U+P1)+9*FMath::Sin(Tau*17*U+P2));
  const double W2=FMath::Sin(Tau*F2*U+37*FMath::Sin(Tau*5*U+P2)+6*FMath::Sin(Tau*23*U+P1));
  const double Envelope=.55+.23*Motion+.12*Detail;
  // Warbles remain embedded in broadband grain. No clean sine dominates, and no
  // syllables, code, 4/7 pulse grouping or borrowed scientific recordings appear.
  const double Warble=(.13*W1+(.035+.075*BW)*W2)*Envelope;
  const double Shimmer=.11*BW*Mid*FMath::Sin(Tau*1327*U+P2);
  return float(.68*Band*(.85+.15*Motion)+Warble+Shimmer);
 }
}

uint32 S47ReceiverTexture::Seed(const FString& Id)
{
 // Explicit UTF-8 FNV-1a, independent of TCHAR width and engine hash revisions.
 FTCHARToUTF8 UTF8(*Id);uint32 H=2166136261u;
 for(int32 I=0;I<UTF8.Length();++I){H^=uint8(UTF8.Get()[I]);H*=16777619u;}
 return H;
}
float S47ReceiverTexture::SourceLevel(float Q,float Gain)
{
 const double Quality=Normal(Q);return float(Normal(Gain)*.80*Quality*Quality);
}
float S47ReceiverTexture::StaticLevel(float Q,float Gain)
{
 return float(Normal(Gain)*(.60-.52*Normal(Q)));
}
float S47ReceiverTexture::Sample(uint32 Identity,double SourceTime,float Q,float Gain,float Bandwidth)
{
 const double U=Position(SourceTime);
 return Layer(Identity,U,ELayer::Source,Bandwidth)*SourceLevel(Q,Gain)
       +Layer(Identity,U,ELayer::Static,Bandwidth)*StaticLevel(Q,Gain);
}
TSharedPtr<FS47AudioBuffer> S47ReceiverTexture::MakeLoop(const TSharedRef<FS47AudioContext>& C,uint32 Identity,ELayer Which,float Bandwidth)
{
 if(!FMath::IsFinite(C->SampleRate)||C->SampleRate<16000||C->SampleRate>192000)return nullptr;
 const int32 Count=FMath::RoundToInt(C->SampleRate*LoopSeconds);
 auto B=C->CreateBuffer(1,Count,C->SampleRate);float* D=B->GetChannelData(0);
 for(int32 I=0;I<Count;++I)D[I]=Layer(Identity,double(I)/Count,Which,Bandwidth);
 return B;
}

bool S47ReceiverTexture::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 auto Check=[&](bool Pass,const TCHAR* What){OK&=Pass;Lines.Add(FString::Printf(TEXT("%s receiver texture: %s"),Pass?TEXT("PASS"):TEXT("FAIL"),What));};
 const uint32 A=Seed(TEXT("3C286")),B=Seed(TEXT("3C48"));
 Check(A==Seed(TEXT("3C286"))&&A!=B&&Seed(TEXT(""))==2166136261u,TEXT("stable source identity; distinct sources differ"));
 bool Finite=true,Repeat=true,Silent=true,Periodic=true;double Difference=0,Open=0,Narrow=0,Energy=0;
 for(int32 I=0;I<48000;++I)
 {
  const double T=I/4000.;const float V=Sample(A,T,.9f,1),Other=Sample(B,T,.9f,1);
  Finite&=FMath::IsFinite(V)&&FMath::Abs(V)<1;
  Repeat&=V==Sample(A,T,.9f,1);Silent&=Sample(A,T,1,0)==0;
  Periodic&=FMath::Abs(V-Sample(A,T+LoopSeconds,.9f,1))<1e-6;
  Difference+=FMath::Square(V-Other);Energy+=V*V;
  Open+=FMath::Square(V-Sample(A,T-1./48000,.9f,1));
  Narrow+=FMath::Square(Sample(A,T,.9f,1,0)-Sample(A,T-1./48000,.9f,1,0));
 }
 Check(Finite&&Silent&&Energy>1,TEXT("finite bounded non-silent audio and exact zero gain"));
 Check(Repeat&&Periodic,TEXT("stateless samples repeat and source time wraps seamlessly"));
 Check(Difference>Energy*.2,TEXT("source identities produce materially distinct texture"));
 Check(Open>Narrow*1.15,TEXT("narrow bandwidth attenuates high-frequency texture"));
 Check(SourceLevel(0,1)==0&&StaticLevel(0,1)>StaticLevel(1,1)&&SourceLevel(1,1)>0,TEXT("tuning reveals source texture through a falling noise floor"));
 auto C=FS47AudioContext::Make(16000);auto S=MakeLoop(C,A,ELayer::Source);auto N=MakeLoop(C,A,ELayer::Static);
 bool Match=S&&N&&S->Length()==192000&&N->Length()==192000&&S->NumberOfChannels()==1;
 if(Match)for(int32 I=0;I<S->Length();I+=127)
 {
  const float Mixed=S->GetChannelData(0)[I]*SourceLevel(.7f,.8f)+N->GetChannelData(0)[I]*StaticLevel(.7f,.8f);
  Match&=FMath::Abs(Mixed-Sample(A,I/16000.,.7f,.8f))<1e-6;
 }
 Check(Match,TEXT("live loop layers reconstruct the same source-time samples as tape playback"));
 return OK;
}

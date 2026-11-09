#include "S47ShedSound.h"
#include "S47Audio.h"
#include "S47Main.h"
#include "S47WebAudio.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47ShedSound,Log,All);

// WEEKLY-CASE-20261109, patch 2. See S47ShedSound.h. The made sounds are plain sums of decaying partials and filtered noise, a few
// hundred samples of arithmetic each, made once per kind and variant and kept in the audio's buffers under "shed:...".
namespace S47ShedLocal
{
 constexpr int32 Variants=4;
 const TCHAR* const Prefixes[]={TEXT("shedknock"),TEXT("shedkick"),TEXT("shedsplinter"),TEXT("shedchain"),TEXT("shedheavy"),TEXT("shedbreath")};
 // One-pole filters, run in place.
 void Low(TArray<float>& S,float Hz,float Rate)
 {
  const float A=1.f-FMath::Exp(-2.f*PI*Hz/Rate);float Y=0;
  for(float& V:S){Y+=A*(V-Y);V=Y;}
 }
 void High(TArray<float>& S,float Hz,float Rate)
 {
  TArray<float> L=S;Low(L,Hz,Rate);
  for(int32 I=0;I<S.Num();++I)S[I]-=L[I];
 }
 void Normalise(TArray<float>& S,float Peak)
 {
  float Top=0;for(float V:S)Top=FMath::Max(Top,FMath::Abs(V));
  if(Top>1e-6f)for(float& V:S)V*=Peak/Top;
 }
 // A decaying partial, added from sample From.
 void Ring(TArray<float>& S,int32 From,float Rate,float Hz,float Tau,float Amp,float Phase=0)
 {
  const int32 Len=FMath::Min(S.Num()-From,int32(Rate*Tau*7));
  for(int32 I=0;I<Len;++I){const float T=I/Rate;S[From+I]+=Amp*FMath::Exp(-T/Tau)*FMath::Sin(2*PI*Hz*T+Phase);}
 }
 // A burst of noise, low-passed, decaying, added from sample From.
 void Burst(TArray<float>& S,int32 From,float Rate,float Tau,float Amp,float LowHz,float HighHz,FRandomStream& R)
 {
  const int32 Len=FMath::Min(S.Num()-From,int32(Rate*Tau*6));
  if(Len<=0)return;
  TArray<float> N;N.SetNumUninitialized(Len);
  for(int32 I=0;I<Len;++I)N[I]=R.FRandRange(-1.f,1.f)*FMath::Exp(-(I/Rate)/Tau);
  if(LowHz>0)Low(N,LowHz,Rate);
  if(HighHz>0)High(N,HighHz,Rate);
  for(int32 I=0;I<Len;++I)S[From+I]+=Amp*N[I];
 }
 void Fade(TArray<float>& S,float Rate,float Out)
 {
  const int32 N=FMath::Min(S.Num(),int32(Rate*Out));
  for(int32 I=0;I<N;++I)S[S.Num()-1-I]*=float(I)/N;
 }
 // Staged recordings of one kind, read once and kept in the audio's buffers (a new audio context reads them again).
 int32 Load(US47Audio* A,S47ShedSound::EKind Kind)
 {
  const FString Prefix=S47ShedSound::Prefix(Kind);
  int32 N=0;
  for(;N<16;++N)
  {
   const FString Key=FString::Printf(TEXT("shed:%s%d"),*Prefix,N);
   if(A&&A->Buf.Contains(Key))continue;
   TArray<uint8> Bytes;
   if(!FFileHelper::LoadFileToArray(Bytes,*(FPaths::ProjectContentDir()/FString::Printf(TEXT("Audio/Raw/night/%s%d.wav"),*Prefix,N)),FILEREAD_Silent))break;
   if(!A)continue;
   if(TSharedPtr<FS47AudioBuffer> Rec=S47Wa::DecodeWav(Bytes))A->Buf.Add(Key,Rec);else break;
  }
  return N;
 }
}

const TCHAR* S47ShedSound::Prefix(EKind Kind){return S47ShedLocal::Prefixes[FMath::Clamp(int32(Kind),0,int32(EKind::Count)-1)];}

int32 S47ShedSound::Recorded(EKind Kind)
{
 int32 N=0;
 for(;N<16;++N)if(!FPaths::FileExists(FPaths::ProjectContentDir()/FString::Printf(TEXT("Audio/Raw/night/%s%d.wav"),Prefix(Kind),N)))break;
 return N;
}

TArray<float> S47ShedSound::Make(EKind Kind,float Hz,int32 Variant)
{
 using namespace S47ShedLocal;
 TArray<float> S;
 const float R=FMath::Max(8000.f,Hz);
 FRandomStream Rand(0x5eed0+int32(Kind)*131+Variant*17);
 const float V=1.f+.06f*(Variant%Variants-1.5f); // a little higher or lower each variant
 switch(Kind)
 {
 case EKind::Knock:
 {
  // Knuckles on a board: the board's low body, its higher modes dying fast, and the tick of the knuckle.
  S.SetNumZeroed(int32(R*.22f));
  Ring(S,0,R,165.f*V,.030f,.9f);Ring(S,0,R,410.f*V,.016f,.55f,.7f);Ring(S,0,R,980.f*V,.007f,.35f,1.3f);
  Burst(S,0,R,.002f,.6f,5000,600,Rand);
  Normalise(S,.85f);
  break;
 }
 case EKind::Kick:
 {
  // A heel against the inside of the door: a heavy low blow, the whole leaf's boom, the boards' rattle in the frame.
  S.SetNumZeroed(int32(R*.9f));
  Ring(S,0,R,62.f*V,.11f,1.f);Ring(S,0,R,118.f*V,.07f,.7f,.4f);Ring(S,0,R,290.f*V,.035f,.4f,.9f);
  Burst(S,0,R,.03f,.9f,900,0,Rand);
  for(int32 K=0;K<6;++K)Burst(S,int32(R*(.04f+K*.035f+Rand.FRandRange(0.f,.02f))),R,.008f,.25f*(1.f-K/7.f),3000,500,Rand);
  Normalise(S,.95f);Fade(S,R,.2f);
  break;
 }
 case EKind::Splinter:
 {
  // The door giving: one great crack, then the boards tearing, cracks coming faster and smaller, a last heavy fall.
  S.SetNumZeroed(int32(R*1.8f));
  Burst(S,0,R,.06f,1.f,4500,200,Rand);Ring(S,0,R,70.f*V,.15f,.9f);
  double T=.08;
  for(int32 K=0;K<38&&T<1.4;++K)
  {
   const float Amp=.75f*FMath::Exp(-float(T)/.6f)+.08f;
   Burst(S,int32(R*T),R,Rand.FRandRange(.003f,.012f),Amp,Rand.FRandRange(2500.f,6000.f),Rand.FRandRange(300.f,900.f),Rand);
   Ring(S,int32(R*T),R,Rand.FRandRange(180.f,520.f),.02f,Amp*.4f);
   T+=Rand.FRandRange(.008,.07)*(1.+T);
  }
  Ring(S,int32(R*1.25f),R,55.f,.12f,.6f);Burst(S,int32(R*1.25f),R,.05f,.5f,700,0,Rand);
  Normalise(S,.97f);Fade(S,R,.15f);
  break;
 }
 case EKind::Chain:
 {
  // Links against the hasp and each other, and the padlock's heavier clack: high inharmonic rings, bunched, uneven.
  S.SetNumZeroed(int32(R*1.1f));
  Ring(S,0,R,920.f*V,.05f,.6f);Ring(S,0,R,2210.f*V,.03f,.4f,.5f);
  double T=.03;
  for(int32 K=0;K<16&&T<.95;++K)
  {
   const float Amp=Rand.FRandRange(.15f,.5f)*(1.f-float(T));
   const float F=Rand.FRandRange(.9f,1.15f)*V;
   for(const float P:{2130.f,3470.f,5240.f,7900.f})Ring(S,int32(R*T),R,P*F,Rand.FRandRange(.015f,.04f),Amp*Rand.FRandRange(.4f,1.f),Rand.FRandRange(0.f,6.f));
   T+=Rand.FRandRange(.015,.11);
  }
  High(S,500,R);Normalise(S,.8f);Fade(S,R,.1f);
  break;
 }
 case EKind::Heavy:
 {
  // Something big inside: a deep thud on the dirt floor, then a slow drag of weight, then a second, softer thud.
  S.SetNumZeroed(int32(R*2.2f));
  Ring(S,0,R,44.f*V,.22f,1.f);Burst(S,0,R,.06f,.6f,350,0,Rand);
  const int32 From=int32(R*.25f),Len=int32(R*1.3f);
  TArray<float> Drag;Drag.SetNumUninitialized(Len);
  for(int32 I=0;I<Len;++I){const float T=I/float(Len);Drag[I]=Rand.FRandRange(-1.f,1.f)*FMath::Sin(PI*T)*(.6f+.4f*FMath::Sin(2*PI*3.3f*T));}
  Low(Drag,260,R);Low(Drag,260,R);
  for(int32 I=0;I<Len&&From+I<S.Num();++I)S[From+I]+=1.6f*Drag[I];
  Ring(S,int32(R*1.6f),R,52.f*V,.16f,.55f);
  Normalise(S,.9f);Fade(S,R,.2f);
  break;
 }
 case EKind::Breath:
 {
  // A slow breath against the boards: in, a held moment, a long wet exhale with a growl low under it. Nothing like words.
  S.SetNumZeroed(int32(R*3.4f));
  TArray<float> N;N.SetNumUninitialized(S.Num());
  for(float& X:N)X=Rand.FRandRange(-1.f,1.f);
  High(N,320,R);Low(N,1300,R);Low(N,2200,R);
  for(int32 I=0;I<S.Num();++I)
  {
   const float T=I/R;
   const float In=T<1.1f?FMath::Sin(PI*T/1.1f)*.55f:0.f;
   const float Out=T>1.35f&&T<3.3f?FMath::Sin(PI*(T-1.35f)/1.95f)*(1.f-.25f*(T-1.35f)):0.f;
   const float Growl=Out*.25f*FMath::Sin(2*PI*(68.f*V)*T)*(.6f+.4f*FMath::Sin(2*PI*7.f*T));
   S[I]=N[I]*(In+Out)+Growl;
  }
  Normalise(S,.7f);Fade(S,R,.1f);
  break;
 }
 default:break;
 }
 return S;
}

bool S47ShedSound::Play(US47Main* Main,EKind Kind,const FVector& AtThree,float Gain,float Rate,int32 Variant,float Lp)
{
 US47Audio* A=Main&&IsValid(Main->Audio)?Main->Audio.Get():nullptr;
 if(!A||!A->Ctx.IsValid()||AtThree.ContainsNaN())return false;
 const int32 Have=S47ShedLocal::Load(A,Kind);
 FString Key;
 if(Have>0)Key=FString::Printf(TEXT("shed:%s%d"),Prefix(Kind),FMath::Abs(Variant)%Have);
 else
 {
  const int32 V=FMath::Abs(Variant)%S47ShedLocal::Variants;
  Key=FString::Printf(TEXT("shed:%s:made%d"),Prefix(Kind),V);
  if(!A->Buf.Contains(Key))
  {
   const float Hz=A->Ctx->SampleRate;
   const TArray<float> S=Make(Kind,Hz,V);
   if(S.IsEmpty())return false;
   TSharedRef<FS47AudioBuffer> B=A->Ctx->CreateBuffer(1,S.Num(),Hz);
   FMemory::Memcpy(B->GetChannelData(0),S.GetData(),S.Num()*sizeof(float));
   A->Buf.Add(Key,B);
   UE_LOG(LogS47ShedSound,Display,TEXT("S47: shed sound %s made (no recording staged), %.2f s"),*Key,S.Num()/Hz);
  }
 }
 FS47Sound O;O.At=AtThree;O.Gain=Gain;O.Rate=FMath::Clamp(Rate,.3f,2.5f);O.Lp=Lp;
 return A->Sfx(Key,O).IsValid();
}

bool S47ShedSound::SelfTest(TArray<FString>& Lines)
{
 bool bGood=true;FString Bad;
 for(int32 K=0;K<int32(EKind::Count);++K)for(int32 V=0;V<S47ShedLocal::Variants;++V)
 {
  const TArray<float> A=Make(EKind(K),48000.f,V),B=Make(EKind(K),48000.f,V);
  float Top=0;bool bFinite=true;for(float X:A){Top=FMath::Max(Top,FMath::Abs(X));bFinite&=FMath::IsFinite(X);}
  const double Secs=A.Num()/48000.;
  const bool bOk=A.Num()>0&&A==B&&bFinite&&Top>.1f&&Top<=1.f&&Secs>.1&&Secs<4.;
  if(!bOk)Bad+=FString::Printf(TEXT(" %s/%d"),Prefix(EKind(K)),V);
  bGood&=bOk;
 }
 Lines.Add(FString(bGood?TEXT("PASS"):TEXT("FAIL"))+TEXT(" shed sounds: knock, kick, splinter, chain, heavy and breath are each made the same every time, in level, under four seconds")+Bad);
 return bGood;
}

#include "S47AlienKinds.h"
#include "S47Audio.h"
#include "S47NightRhythm.h"
#include "S47WebAudio.h"

// D48-20261110 (D46-15, D46-17). Everything up to the player is pure (S47AlienKinds.h).
namespace S47AlienKindsLocal
{
 constexpr double TwoPi=2.*UE_DOUBLE_PI;
 // The game's own: one draw sequence per take, the same on every machine.
 struct FRng
 {
  uint32 S;
  explicit FRng(uint32 Seed):S(Seed?Seed:0x9e3779b9u){for(int32 I=0;I<4;++I)Next();}
  float Next(){S^=S<<13;S^=S>>17;S^=S<<5;return float(S&0xffffffu)/16777216.f;}
  double Range(double A,double B){return A+(B-A)*double(Next());}
 };
 // Up over A seconds, down over the last R.
 double Edge(double T,double Len,double A,double R){if(T<0||T>=Len)return 0;return FMath::Min3(1.,A>0?T/A:1.,R>0?(Len-T)/R:1.);}
 int32 First(double At,float Rate){return FMath::Max(0,int32(At*Rate));}
 int32 Last(double At,float Rate,int32 N){return FMath::Clamp(int32(At*Rate)+1,0,N);}
 // A tone from At for Len, gliding from F0 to F1 (evenly in pitch), with a second and third partial and a slight vibrato.
 void Tone(TArray<float>& O,float Rate,double At,double Len,double F0,double F1,double Amp,double H2,double H3,double A,double R,double Vib=0,double VibHz=0)
 {
  if(Len<=0||F0<=0||F1<=0)return;
  const int32 I1=Last(At+Len,Rate,O.Num());const double Glide=F1/F0;double Ph=0;
  for(int32 I=First(At,Rate);I<I1;++I)
  {
   const double T=double(I)/Rate-At;if(T<0)continue;
   double F=Glide==1.?F0:F0*FMath::Pow(Glide,FMath::Clamp(T/Len,0.,1.));
   if(Vib!=0.)F*=1.+Vib*FMath::Sin(TwoPi*VibHz*T);
   Ph+=F/Rate;Ph-=FMath::FloorToDouble(Ph);
   double V=FMath::Sin(TwoPi*Ph);
   if(H2!=0.)V+=H2*FMath::Sin(2*TwoPi*Ph);
   if(H3!=0.)V+=H3*FMath::Sin(3*TwoPi*Ph);
   O[I]+=float(Amp*Edge(T,Len,A,R)*V);
  }
 }
 // A deep pulse: swells over Attack, dies away with Tau; its partials a little off true, so that it is not a note.
 void Thump(TArray<float>& O,float Rate,double At,double F,double Amp,double Attack,double Tau)
 {
  const double Len=Attack+Tau*5;const int32 I1=Last(At+Len,Rate,O.Num());
  for(int32 I=First(At,Rate);I<I1;++I)
  {
   const double T=double(I)/Rate-At;if(T<0)continue;
   const double S=T<Attack?FMath::Sin(.5*UE_DOUBLE_PI*T/Attack):1.;
   const double E=T<Attack?S*S:FMath::Exp(-(T-Attack)/Tau);
   O[I]+=float(Amp*E*(FMath::Sin(TwoPi*F*T)+.4*FMath::Sin(TwoPi*2.*F*T)+.15*FMath::Sin(TwoPi*3.03*F*T)));
  }
 }
 // One knock of metal: a few inharmonic partials, gone in a few hundredths of a second.
 void Ring(TArray<float>& O,float Rate,double At,double Fc,double Amp,double Tau)
 {
  static const double Ratio[4]={1.,1.58,2.37,3.11},Part[4]={1.,.6,.4,.25};
  const int32 I1=Last(At+Tau*6,Rate,O.Num());
  for(int32 I=First(At,Rate);I<I1;++I)
  {
   const double T=double(I)/Rate-At;if(T<0)continue;
   double V=0;for(int32 K=0;K<4;++K)V+=Part[K]*FMath::Sin(TwoPi*Fc*Ratio[K]*T);
   O[I]+=float(Amp*FMath::Exp(-T/Tau)*V*FMath::Min(1.,T/.0008));
  }
 }
 // A creak: knocks that come faster and then slower again, as a hinge or a hull under strain.
 void Creak(TArray<float>& O,float Rate,FRng& R,double At,double Len,double Fc,double Amp)
 {
  for(double T=At;T<At+Len;)
  {
   const double K=(T-At)/Len,PerSecond=25.+45.*FMath::Sin(UE_DOUBLE_PI*K);
   Ring(O,Rate,T,Fc*R.Range(.97,1.03),Amp*R.Range(.5,1.),.018);
   T+=R.Range(.8,1.2)/PerSecond;
  }
 }
 // Breath: band-limited noise drawn in and let out (Cycle seconds a breath); Buzz rasps it at 45 a second.
 void Breath(TArray<float>& O,float Rate,FRng& R,double Lo,double Hi,double Cycle,double Amp,double Buzz)
 {
  const double ALo=1.-FMath::Exp(-TwoPi*Lo/Rate),AHi=1.-FMath::Exp(-TwoPi*Hi/Rate),Phase0=R.Range(0.,1.);
  double Low=0,High=0;
  for(int32 I=0;I<O.Num();++I)
  {
   const double T=double(I)/Rate,X=2.*R.Next()-1.;
   Low+=ALo*(X-Low);High+=AHi*((X-Low)-High);
   const double P=FMath::Fmod(T/Cycle+Phase0,1.);
   const double In=P<.4?FMath::Sin(UE_DOUBLE_PI*P/.4):0.,Out=P>=.45&&P<.95?FMath::Sin(UE_DOUBLE_PI*(P-.45)/.5):0.;
   const double E=.7*In*In+Out*Out;
   const double Rasp=Buzz>0?(1.-Buzz)+Buzz*(FMath::Fmod(T*45.,1.)<.35?1.6:.15):1.;
   O[I]+=float(Amp*E*Rasp*High*3.);
  }
 }
 void SynthRoswell(TArray<float>& O,float Rate,FRng& R,double Len)
 {
  // Ladders that rise and end on a question, each a little higher than the last, a pause after each as if for an answer.
  double T=R.Range(.2,.4);const double Base=R.Range(470.,640.),Step=R.Range(1.11,1.19);
  for(int32 Phrase=0;T<Len-1.;++Phrase)
  {
   const int32 Steps=3+FMath::Min(2,int32(R.Next()*3.f));
   double F=Base*FMath::Pow(1.06,double(Phrase));
   for(int32 S=0;S<Steps&&T<Len-.5;++S)
   {
    const bool bAsk=S==Steps-1;const double D=R.Range(.12,.2)*(bAsk?1.6:1.);
    Tone(O,Rate,T,D,F,bAsk?F*R.Range(1.25,1.45):F*1.015,.5,.22,.08,.012,.03,.006,6.5);
    T+=D+.035;F*=Step;
   }
   T+=R.Range(.55,.95);
  }
  // Not a bird and not a whistle: the whole of it rung at an odd low rate.
  for(int32 I=0;I<O.Num();++I)O[I]*=float(.78+.22*FMath::Sin(TwoPi*37.*I/Rate));
 }
 void SynthTall(TArray<float>& O,float Rate,FRng& R,double Len)
 {
  const double P=R.Range(1.15,1.45),Fp=R.Range(50.,60.);
  for(double T=R.Range(.1,.4);T<Len-.6;T+=P*R.Range(.95,1.05))Thump(O,Rate,T,Fp,.9,.18,.33);
  Breath(O,Rate,R,250.,900.,R.Range(3.4,4.4),.32,0.);
  // Two tones a few beats a second apart, and the tritone over them, faint.
  const double Fb=R.Range(104.,116.);
  Tone(O,Rate,0,Len,Fb,Fb,.22,0,0,1.6,1.2);Tone(O,Rate,0,Len,Fb+R.Range(2.6,3.8),Fb+R.Range(2.6,3.8),.22,0,0,1.6,1.2);Tone(O,Rate,0,Len,Fb*1.4142,Fb*1.4142,.08,0,0,2.,1.2);
  const int32 Creaks=3+FMath::Min(1,int32(R.Next()*2.f));
  for(int32 K=0;K<Creaks;++K)Creak(O,Rate,R,R.Range(.6,Len-1.),R.Range(.25,.5),R.Range(650.,1050.),.22);
  // The drop: down to nearly nothing.
  Tone(O,Rate,R.Range(2.4,4.),1.3,72.,26.,.8,.2,0,.1,.5);
 }
 void SynthPale(TArray<float>& O,float Rate,FRng& R,double Len)
 {
  // A heart's double pulse, quicker than the Tall One's, and a breath that rasps close to the set.
  const double P=R.Range(.85,1.05),Fp=R.Range(42.,50.);
  for(double T=R.Range(.05,.3);T<Len-.5;T+=P*R.Range(.96,1.04)){Thump(O,Rate,T,Fp,.85,.05,.16);Thump(O,Rate,T+.21,Fp*1.05,.55,.04,.14);}
  Breath(O,Rate,R,700.,2400.,R.Range(1.9,2.5),.38,.6);
  const double Fb=R.Range(226.,240.);
  Tone(O,Rate,0,Len,Fb,Fb,.16,0,0,1.,1.);Tone(O,Rate,0,Len,Fb*1.0595,Fb*1.0595,.16,0,0,1.,1.);Tone(O,Rate,0,Len,Fb+R.Range(2.5,3.5),Fb+R.Range(2.5,3.5),.12,0,0,1.,1.);
  const int32 Creaks=5+FMath::Min(2,int32(R.Next()*3.f));
  for(int32 K=0;K<Creaks;++K)Creak(O,Rate,R,R.Range(.4,Len-.8),R.Range(.15,.35),R.Range(1300.,2000.),.2);
  Tone(O,Rate,Len-R.Range(1.6,1.9),1.2,64.,22.,.7,.2,0,.08,.45);
 }
 void SynthNordic(TArray<float>& O,float Rate,FRng& R,double Len)
 {
  // True intervals, a major chord, then the same a fourth higher; the third sinks and the fifth rises while they hold, so that
  // what began as the purest thing on the band ends a little wrong.
  static const double Ratio[4]={1.,1.25,1.5,2.},Amp[4]={.5,.38,.34,.2};
  const double Fr=R.Range(190.,250.),Split=R.Range(3.,3.6);
  for(int32 Chord=0;Chord<2;++Chord)
  {
   const double At=Chord==0?.05:Split-.4,Len1=Chord==0?Split:Len-At-.05,Root=Chord==0?Fr:Fr*4./3.;
   const double Third=Chord==0?-40.:-55.,Fifth=Chord==0?18.:25.;
   for(int32 V=0;V<4;++V)
   {
    const double F=Root*Ratio[V],Cents=V==1?Third:V==2?Fifth:0.;
    Tone(O,Rate,At+.25*V,Len1-.25*V,F,F*FMath::Pow(2.,Cents/1200.),Amp[V],.06,0,1.+.25*V,.8,V==3?.003:0.,4.2);
   }
  }
  Tone(O,Rate,.8,Len-1.,Fr*3.,Fr*3.,.05,0,0,1.2,1.,.004,3.);
 }
 void SynthCosmic(TArray<float>& O,float Rate,FRng& R,double Len)
 {
  // It counts: groups of pips (1 2 3 5 7, or 1 4 9, or 1 1 2 3 5), the groups in two pitches by turns, and then the next of
  // them quicker and lower, as if for the listener to see it. Over a steady carrier.
  static const int32 Seq[3][6]={{1,2,3,5,7,11},{1,4,9,16,0,0},{1,1,2,3,5,8}};
  static const int32 Groups[3]={5,3,5};
  const int32 Which=FMath::Min(2,int32(R.Next()*3.f));
  const double Fa=R.Range(980.,1120.);
  double T=.3;
  for(int32 G=0;G<Groups[Which];++G)
  {
   for(int32 K=0;K<Seq[Which][G]&&T<Len-.4;++K){Tone(O,Rate,T,.045,G%2?Fa*1.5:Fa,G%2?Fa*1.5:Fa,.55,0,.15,.004,.015);T+=.1;}
   T+=.38;
  }
  T+=.25;
  for(int32 K=0;K<Seq[Which][Groups[Which]]&&T<Len-.3;++K){Tone(O,Rate,T,.035,Fa*.75,Fa*.75,.5,0,.15,.004,.012);T+=.07;}
  Tone(O,Rate,0,Len,Fa/8.,Fa/8.,.12,0,0,1.,1.,.002,.2);
 }
}

const S47AlienKinds::FTemper& S47AlienKinds::Temper(ES47AlienKind Kind)
{
 // Horror weighs most: the Tall One and the Pale One lean on well over half the nights.
 static const FTemper Tempers[int32(ES47AlienKind::Num)]={
  {TEXT("roswell"),TEXT("curious"),.24f,38.,false},
  {TEXT("tall"),TEXT("malicious"),.30f,0.,true},
  {TEXT("pale"),TEXT("malicious"),.28f,0.,true},
  {TEXT("nordic"),TEXT("harmonic"),.10f,0.,false},
  {TEXT("cosmic"),TEXT("enlightening"),.08f,0.,false},
 };
 return Tempers[FMath::Clamp(int32(Kind),0,int32(ES47AlienKind::Num)-1)];
}

ES47AlienKind S47AlienKinds::NightLean(int32 Seed,int32 Night)
{
 const float Roll=FS47NightRhythm::Roll(Seed,Night,0x4b494e44u);
 float Sum=0;
 for(int32 K=0;K<int32(ES47AlienKind::Num);++K){Sum+=Temper(ES47AlienKind(K)).Weight;if(Roll<Sum)return ES47AlienKind(K);}
 return ES47AlienKind::Tall;
}

int32 S47AlienKinds::VisitCode(ES47AlienKind Kind){return Kind==ES47AlienKind::Roswell?0:Kind==ES47AlienKind::Pale?1:Kind==ES47AlienKind::Tall?2:-1;}
ES47AlienKind S47AlienKinds::OfVisit(int32 Code){return Code==1?ES47AlienKind::Pale:Code==2?ES47AlienKind::Tall:ES47AlienKind::Roswell;}

int32 S47AlienKinds::LeanVisit(int32 Picked,ES47AlienKind Lean,double Hour,float Roll)
{
 const int32 Code=VisitCode(Lean);
 if(Code<0||Roll>=LeanShare)return Picked;
 // The Tall One keeps FS47FarmSite::PickWho's own hour.
 if(Code==2&&Hour<3)return Picked;
 return Code;
}

FString S47AlienKinds::LeanFigure(ES47AlienKind Lean,float Roll)
{
 // Of a hundred figures on the rise: on a Nordic night 70 his, 5 the Cosmic one's; the other way round on a Cosmic night; on a
 // Roswell night a quarter each; on a Tall or Pale night mostly the shade.
 float N=.15f,C=.15f;
 if(Lean==ES47AlienKind::Nordic){N=.7f;C=.05f;}
 else if(Lean==ES47AlienKind::Cosmic){N=.05f;C=.7f;}
 else if(Lean==ES47AlienKind::Roswell){N=.25f;C=.25f;}
 return Roll<N?FString(TEXT("nordic")):Roll<N+C?FString(TEXT("cosmic")):FString();
}

float S47AlienKinds::TierLevel(ES47Allowed Tier){return Tier==ES47Allowed::Full?1.f:Tier==ES47Allowed::Glimpse?.7f:.4f;}

uint32 S47AlienKinds::TakeSeed(int32 GameSeed,const FString& JobId)
{
 uint32 H=2166136261u^(uint32(GameSeed)*2654435761u);
 for(const TCHAR C:JobId){H^=uint32(C);H*=16777619u;}
 return H?H:1u;
}

void S47AlienKinds::Render(ES47AlienKind Kind,uint32 Seed,float Rate,float Level,TArray<float>& Out)
{
 using namespace S47AlienKindsLocal;
 Out.Reset();
 if(!(Rate>=4000.f&&Rate<=192000.f))return;
 const int32 N=FMath::RoundToInt32(SignalSeconds*Rate);Out.SetNumZeroed(N);
 if(!(Level>0.f))return;
 FRng R(Seed^(0x51a70000u+uint32(Kind)*0x01000193u));
 switch(Kind)
 {
  case ES47AlienKind::Roswell:SynthRoswell(Out,Rate,R,SignalSeconds);break;
  case ES47AlienKind::Tall:SynthTall(Out,Rate,R,SignalSeconds);break;
  case ES47AlienKind::Pale:SynthPale(Out,Rate,R,SignalSeconds);break;
  case ES47AlienKind::Nordic:SynthNordic(Out,Rate,R,SignalSeconds);break;
  default:SynthCosmic(Out,Rate,R,SignalSeconds);break;
 }
 // As the receiver would pass it: no DC, nothing over the voice band, a slow fade in and out of the signal, and its own edges.
 const double AHp=1.-FMath::Exp(-TwoPi*35./Rate),ALp=1.-FMath::Exp(-TwoPi*3200./Rate),Fade=R.Range(0.,TwoPi);
 double Dc=0,Lp1=0,Lp2=0,Sum=0;
 for(int32 I=0;I<N;++I)
 {
  const double T=double(I)/Rate;
  Dc+=AHp*(Out[I]-Dc);const double X=Out[I]-Dc;Lp1+=ALp*(X-Lp1);Lp2+=ALp*(Lp1-Lp2);
  const double In=FMath::Clamp(T/.3,0.,1.),Away=FMath::Clamp((SignalSeconds-T)/.45,0.,1.);
  const double V=Lp2*(.82+.18*FMath::Sin(TwoPi*.21*T+Fade))*(.5-.5*FMath::Cos(UE_DOUBLE_PI*In))*(.5-.5*FMath::Cos(UE_DOUBLE_PI*Away));
  Out[I]=float(V);Sum+=V*V;
 }
 // Levelled: its rms to Level of RmsFull, its knocks and pulses rounded off softly so that no peak reaches Level of PeakFull.
 const double Rms=FMath::Sqrt(Sum/FMath::Max(1,N));if(Rms<=1e-12)return;
 const double Cap=double(PeakFull)*Level;
 double K=double(RmsFull)*Level/Rms,Peak=0;
 for(int32 I=0;I<N;++I)Peak=FMath::Max(Peak,FMath::Abs(double(Out[I])*K));
 if(Peak>2.*Cap)K*=2.*Cap/Peak;
 for(int32 I=0;I<N;++I)
 {
  const double X=double(Out[I])*K/Cap,E=FMath::Exp(-2.*FMath::Abs(X));
  Out[I]=float(Cap*(X<0?-1.:1.)*(1.-E)/(1.+E)*.999);
 }
}

namespace S47AlienKindsLocal
{
 // Rendered at this rate and played at the context's (the receiver passes nothing over 3.2 kHz): half the work at 48 kHz.
 constexpr float PlayRate=24000.f;
 struct FVoice
 {
  // Weak only: a started source is held by its context, and nothing here keeps a context or a graph alive.
  TWeakPtr<FS47BufferSourceNode> Src;TWeakPtr<FS47AudioContext> Ctx;
  // What the source plays, and what the buffer was rendered for (rendered when the tape starts rolling, before the window).
  uint32 SrcSeed=0;int32 SrcKind=-1;float SrcLevel=-1;
  TSharedPtr<FS47AudioBuffer> Buf;uint32 BufSeed=0;int32 BufKind=-1;float BufLevel=-1;
  double LastCursor=-1;bool bSpent=false;
 };
 TMap<FString,FVoice>& Voices(){static TMap<FString,FVoice> V;return V;}
 void Silence(FVoice& V)
 {
  const TSharedPtr<FS47BufferSourceNode> S=V.Src.Pin();const TSharedPtr<FS47AudioContext> C=V.Ctx.Pin();
  if(S&&C){FS47AudioContext::FBatch Batch(*C);S->Stop();S->Disconnect();}
  V.Src.Reset();
 }
 bool Prepared(FVoice& V,FS47AudioContext& Ctx,ES47AlienKind Kind,uint32 Seed,float Level)
 {
  if(V.Buf&&V.BufSeed==Seed&&V.BufKind==int32(Kind)&&V.BufLevel==Level)return true;
  TArray<float> Pcm;S47AlienKinds::Render(Kind,Seed,PlayRate,Level,Pcm);if(Pcm.IsEmpty())return false;
  const TSharedRef<FS47AudioBuffer> B=Ctx.CreateBuffer(1,Pcm.Num(),PlayRate);
  FMemory::Memcpy(B->GetChannelData(0),Pcm.GetData(),Pcm.Num()*sizeof(float));
  V.Buf=B;V.BufSeed=Seed;V.BufKind=int32(Kind);V.BufLevel=Level;return true;
 }
}

void S47AlienKinds::Hush(const TCHAR* Key)
{
 if(S47AlienKindsLocal::FVoice* V=S47AlienKindsLocal::Voices().Find(Key)){S47AlienKindsLocal::Silence(*V);V->bSpent=false;V->LastCursor=-1;}
}

void S47AlienKinds::Follow(US47Audio* Audio,const TCHAR* Key,bool bRolling,double Cursor,ES47AlienKind Kind,uint32 Seed,float Level)
{
 using namespace S47AlienKindsLocal;
 FVoice* Found=Voices().Find(Key);
 if(!Found){if(!bRolling)return;Found=&Voices().Add(Key);}
 FVoice& V=*Found;
 const bool bIn=bRolling&&Level>0.f&&FMath::IsFinite(Cursor)&&Cursor>=SignalFrom&&Cursor<SignalFrom+SignalSeconds-.05;
 // A seek at the deck (back, or on by more than a frame could carry it), or another take: start again from where the tape is.
 const bool bJumped=V.LastCursor>=0&&(Cursor<V.LastCursor-.02||Cursor>V.LastCursor+.5);
 const bool bOther=V.SrcSeed!=Seed||V.SrcKind!=int32(Kind)||V.SrcLevel!=Level;
 V.LastCursor=bRolling?Cursor:-1.;
 if(!bIn||bJumped||bOther){Silence(V);V.bSpent=false;}
 if(!bRolling||!(Level>0.f)||!Audio||!Audio->Ctx||!Audio->SfxBus||Audio->Ctx->State()!=TEXT("running"))return;
 const TSharedRef<FS47AudioContext> Ctx=Audio->Ctx.ToSharedRef();
 if(!Prepared(V,*Ctx,Kind,Seed,Level)||!bIn||V.bSpent)return;
 FS47AudioContext::FBatch Batch(*Ctx);
 const TSharedRef<FS47BufferSourceNode> S=Ctx->CreateBufferSource();S->Buffer=V.Buf;S->Connect(Audio->SfxBus);
 S->Start(Ctx->CurrentTime(),Cursor-SignalFrom);
 V.Src=S;V.Ctx=Ctx;V.SrcSeed=Seed;V.SrcKind=int32(Kind);V.SrcLevel=Level;V.bSpent=true;
}

bool S47AlienKinds::SelfTest(TArray<FString>& Lines)
{
 using namespace S47AlienKindsLocal;
 bool OK=true;
 const auto Check=[&](bool B,const FString& Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s alien kinds: %s"),B?TEXT("PASS"):TEXT("FAIL"),*Why));};
 constexpr int32 Kinds=int32(ES47AlienKind::Num);
 // The tempers: weights that sum to one, horror the most of them, the curious ones slow to come, the malicious ones that stalk.
 {
  float Sum=0;for(int32 K=0;K<Kinds;++K)Sum+=Temper(ES47AlienKind(K)).Weight;
  const float Horror=Temper(ES47AlienKind::Tall).Weight+Temper(ES47AlienKind::Pale).Weight;
  Check(FMath::IsNearlyEqual(Sum,1.f,1e-4f)&&Horror>.5f&&Temper(ES47AlienKind::Roswell).HuntAfter>22.&&!Temper(ES47AlienKind::Roswell).bStalks&&Temper(ES47AlienKind::Tall).bStalks&&Temper(ES47AlienKind::Pale).bStalks,
   TEXT("five tempers: the Tall One and the Pale One stalk and lean on most nights; the Roswell ones watch longer before they come"));
 }
 // The night's lean: the seed's, the same every time, near the weights over many nights.
 {
  int32 Count[Kinds]={};bool bSame=true;const int32 N=40*30;
  for(int32 Seed=1;Seed<=40;++Seed)for(int32 Night=1;Night<=30;++Night){const ES47AlienKind L=NightLean(Seed,Night);++Count[int32(L)];bSame&=L==NightLean(Seed,Night);}
  bool bNear=true;for(int32 K=0;K<Kinds;++K)bNear&=FMath::Abs(float(Count[K])/N-Temper(ES47AlienKind(K)).Weight)<.05f;
  Check(bSame&&bNear,FString::Printf(TEXT("the night's lean by the seed: Roswell %d, Tall %d, Pale %d, Nordic %d, Cosmic %d of %d"),Count[0],Count[1],Count[2],Count[3],Count[4],N));
 }
 // The site visit leans toward the night's kind, keeps the Tall One's hour, and Nordic and Cosmic nights leave it as drawn.
 {
  bool bLean=LeanVisit(0,ES47AlienKind::Pale,4.,.1f)==1&&LeanVisit(1,ES47AlienKind::Roswell,4.,.1f)==0&&LeanVisit(0,ES47AlienKind::Tall,4.,.1f)==2;
  bLean&=LeanVisit(0,ES47AlienKind::Tall,2.5,.1f)==0&&LeanVisit(2,ES47AlienKind::Pale,4.,.9f)==2&&LeanVisit(1,ES47AlienKind::Nordic,4.,0.f)==1&&LeanVisit(2,ES47AlienKind::Cosmic,4.,0.f)==2;
  bLean&=OfVisit(VisitCode(ES47AlienKind::Pale))==ES47AlienKind::Pale&&OfVisit(VisitCode(ES47AlienKind::Tall))==ES47AlienKind::Tall&&VisitCode(ES47AlienKind::Nordic)<0;
  Check(bLean,TEXT("a site visit leans to the night's kind (not the Tall One before three; Nordic and Cosmic never come to a site)"));
  int32 Nordic=0,Cosmic=0,Shade=0;
  for(int32 I=0;I<1000;++I){const FString F=LeanFigure(ES47AlienKind::Nordic,(I+.5f)/1000.f);Nordic+=F==TEXT("nordic");Cosmic+=F==TEXT("cosmic");}
  for(int32 I=0;I<1000;++I)Shade+=LeanFigure(ES47AlienKind::Tall,(I+.5f)/1000.f).IsEmpty();
  Check(Nordic>600&&Cosmic<100&&Shade>600,FString::Printf(TEXT("the figure on the rise: a Nordic night %d Nordic, %d Cosmic of 1000; a Tall night %d shades"),Nordic,Cosmic,Shade));
 }
 // The signatures: each renders, stays inside its levels, is the same for the same take and another for another take.
 {
  constexpr float Rate=16000.f;
  TArray<float> Pcm[Kinds];bool bRenders=true,bLevels=true,bEdges=true,bSame=true,bSeeded=true;FString Levels;
  for(int32 K=0;K<Kinds;++K)
  {
   const ES47AlienKind Kind=ES47AlienKind(K);
   Render(Kind,4747u,Rate,1.f,Pcm[K]);
   double Sum=0,Peak=0;bool bFinite=true;for(float V:Pcm[K]){bFinite&=FMath::IsFinite(V);Sum+=double(V)*V;Peak=FMath::Max(Peak,double(FMath::Abs(V)));}
   const double Rms=FMath::Sqrt(Sum/FMath::Max(1,Pcm[K].Num()));
   bRenders&=bFinite&&Pcm[K].Num()==FMath::RoundToInt32(SignalSeconds*Rate)&&Rms>0;
   bLevels&=Peak<=PeakFull+1e-5&&Rms<=RmsFull*1.001&&Rms>=RmsFull*.25;
   bEdges&=FMath::Abs(Pcm[K][0])<1e-4f&&FMath::Abs(Pcm[K].Last())<1e-4f;
   Levels+=FString::Printf(TEXT(" %s rms %.4f peak %.4f;"),Temper(Kind).Id,Rms,Peak);
   TArray<float> Again,Other,Faint;Render(Kind,4747u,Rate,1.f,Again);Render(Kind,4748u,Rate,1.f,Other);Render(Kind,4747u,Rate,TierLevel(ES47Allowed::Hint),Faint);
   bSame&=Again==Pcm[K];
   double Diff=0;for(int32 I=0;I<Other.Num()&&I<Pcm[K].Num();++I)Diff+=FMath::Abs(double(Other[I])-Pcm[K][I]);bSeeded&=Diff>1.;
   double FaintPeak=0;for(float V:Faint)FaintPeak=FMath::Max(FaintPeak,double(FMath::Abs(V)));bLevels&=FaintPeak<=PeakFull*TierLevel(ES47Allowed::Hint)+1e-5&&FaintPeak<Peak;
  }
  Check(bRenders,TEXT("each kind's signature renders, finite, its full length"));
  Check(bLevels,TEXT("each stays under the station's noise: rms at most .026, peaks at most .09, fainter before midnight")+Levels);
  Check(bEdges&&bSame&&bSeeded,TEXT("each fades in and out, is the same for the same take, and another for another take"));
  // Distinct: by where their sound lies (sixteen bands, 30 Hz to 3 kHz) and how it moves (forty slices of the level).
  TArray<double> Feat[Kinds];
  for(int32 K=0;K<Kinds;++K)
  {
   const TArray<float>& X=Pcm[K];double Norm=0;
   for(int32 B=0;B<16;++B)
   {
    const double F=30.*FMath::Pow(100.,B/15.),Cw=2.*FMath::Cos(TwoPi*F/Rate);double S1=0,S2=0;
    for(float V:X){const double S0=V+Cw*S1-S2;S2=S1;S1=S0;}
    const double E=FMath::Sqrt(FMath::Max(0.,S1*S1+S2*S2-Cw*S1*S2));Feat[K].Add(FMath::Loge(1.+E));Norm+=FMath::Square(Feat[K].Last());
   }
   for(int32 B=0;B<16;++B)Feat[K][B]/=FMath::Max(1e-9,FMath::Sqrt(Norm));
   const int32 Slice=X.Num()/40;TArray<double> Env;double Mean=0,EnvNorm=0;
   for(int32 S=0;S<40;++S){double E=0;for(int32 I=S*Slice;I<(S+1)*Slice;++I)E+=double(X[I])*X[I];Env.Add(FMath::Sqrt(E/Slice));Mean+=Env.Last()/40;}
   for(double& E:Env){E-=Mean;EnvNorm+=E*E;}
   for(double E:Env)Feat[K].Add(E/FMath::Max(1e-12,FMath::Sqrt(EnvNorm)));
  }
  double Most=-1;FString Pair;
  for(int32 A=0;A<Kinds;++A)for(int32 B=A+1;B<Kinds;++B)
  {
   double Dot=0;for(int32 I=0;I<Feat[A].Num();++I)Dot+=Feat[A][I]*Feat[B][I];
   const double Like=Dot/2.;if(Like>Most){Most=Like;Pair=FString::Printf(TEXT("%s and %s"),Temper(ES47AlienKind(A)).Id,Temper(ES47AlienKind(B)).Id);}
  }
  Check(Most<.9,FString::Printf(TEXT("each kind sounds its own: the nearest two (%s) alike %.2f of 1"),*Pair,Most));
  // Their own marks: the Tall One and the Pale One deep (under 120 Hz), the Cosmic one in pips that come and go.
  const auto Low=[&](int32 K){double L=0,All=0;for(int32 B=0;B<16;++B){const double F=30.*FMath::Pow(100.,B/15.);All+=Feat[K][B];if(F<120.)L+=Feat[K][B];}return All>0?L/All:0.;};
  const double TallLow=Low(1),PaleLow=Low(2),RoswellLow=Low(0);
  int32 Pips=0;{const TArray<float>& X=Pcm[4];const int32 W=int32(Rate*.01);bool bUp=false;double Top=0;for(float V:X)Top=FMath::Max(Top,double(FMath::Abs(V)));
   for(int32 I=0;I+W<=X.Num();I+=W){double E=0;for(int32 J=I;J<I+W;++J)E=FMath::Max(E,double(FMath::Abs(X[J])));const bool bNow=E>Top*.45;Pips+=bNow&&!bUp;bUp=bNow;}}
  Check(TallLow>RoswellLow&&PaleLow>RoswellLow&&Pips>=10,FString::Printf(TEXT("the Tall and Pale ones deep (%.2f, %.2f of their sound under 120 Hz, the Roswell ones %.2f), the Cosmic one counts (%d pips)"),TallLow,PaleLow,RoswellLow,Pips));
 }
 // The player: nothing without an audio graph, and a hush that is safe to ask for at any time.
 {
  Follow(nullptr,TEXT("selftest"),true,10.,ES47AlienKind::Tall,1u,1.f);Hush(TEXT("selftest"));Hush(TEXT("never-started"));
  Check(TakeSeed(47,TEXT("FF-001"))==TakeSeed(47,TEXT("FF-001"))&&TakeSeed(47,TEXT("FF-001"))!=TakeSeed(47,TEXT("FF-002"))&&TakeSeed(47,TEXT("FF-001"))!=TakeSeed(48,TEXT("FF-001")),
   TEXT("a take's seed is its own: the game's seed and the job"));
 }
 // Offline through the game's own graph: a buffer source on a gain, as the SFX bus has it, gives back the rendered samples.
 {
  const TSharedRef<FS47AudioContext> Ctx=FS47AudioContext::Make(16000.f);
  TArray<float> Pcm;Render(ES47AlienKind::Nordic,99u,Ctx->SampleRate,1.f,Pcm);
  const TSharedRef<FS47AudioBuffer> B=Ctx->CreateBuffer(1,Pcm.Num(),Ctx->SampleRate);FMemory::Memcpy(B->GetChannelData(0),Pcm.GetData(),Pcm.Num()*sizeof(float));
  const TSharedRef<FS47GainNode> Bus=Ctx->CreateGain();Bus->Connect(Ctx->Destination);
  const TSharedRef<FS47BufferSourceNode> S=Ctx->CreateBufferSource();S->Buffer=B;S->Connect(Bus);S->Start(0,2.);
  TArray<float> Got;Got.SetNumZeroed(16000);Ctx->Render(Got.GetData(),Got.Num(),1);
  double Err=0,Energy=0;for(int32 I=0;I<Got.Num();++I){Err=FMath::Max(Err,double(FMath::Abs(Got[I]-Pcm[32000+I])));Energy+=FMath::Abs(Got[I]);}
  Check(Energy>1.&&Err<1e-4,FString::Printf(TEXT("the signature plays through the audio graph from an offset as rendered (largest error %.6f)"),Err));
 }
 // As the game plays it: rendered at PlayRate, played in a 48 kHz context, still under its peak.
 {
  const TSharedRef<FS47AudioContext> Ctx=FS47AudioContext::Make(48000.f);
  TArray<float> Pcm;Render(ES47AlienKind::Tall,7u,PlayRate,1.f,Pcm);
  const TSharedRef<FS47AudioBuffer> B=Ctx->CreateBuffer(1,Pcm.Num(),PlayRate);FMemory::Memcpy(B->GetChannelData(0),Pcm.GetData(),Pcm.Num()*sizeof(float));
  const TSharedRef<FS47BufferSourceNode> S=Ctx->CreateBufferSource();S->Buffer=B;S->Connect(Ctx->Destination);S->Start(0,3.);
  TArray<float> Got;Got.SetNumZeroed(24000);Ctx->Render(Got.GetData(),Got.Num(),1);
  double Peak=0,Energy=0;bool bFinite=true;for(float V:Got){bFinite&=FMath::IsFinite(V);Peak=FMath::Max(Peak,double(FMath::Abs(V)));Energy+=FMath::Abs(V);}
  Check(bFinite&&Energy>1.&&Peak<=PeakFull+1e-3,FString::Printf(TEXT("rendered at %.0f Hz it plays at 48 kHz, peak %.4f"),PlayRate,Peak));
 }
 return OK;
}

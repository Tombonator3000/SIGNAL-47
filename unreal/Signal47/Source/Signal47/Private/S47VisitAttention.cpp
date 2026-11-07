#include "S47VisitAttention.h"

const TCHAR* FS47VisitAttention::Name(EDo Value)
{
 switch(Value){case EDo::Watch:return TEXT("watch");case EDo::Back:return TEXT("back");case EDo::Blink:return TEXT("blink");case EDo::Turn:return TEXT("turn");default:return TEXT("none");}
}

void FS47VisitAttention::Let(double LampPause,double SoundPause)
{
 Do=EDo::None;T=0;LampWait=FMath::Max(LampWait,LampPause);SoundWait=FMath::Max(SoundWait,SoundPause);
}

void FS47VisitAttention::Step(double Dt,const FIn& In,float Roll)
{
 T+=Dt;LampWait=FMath::Max(0.,LampWait-Dt);SoundWait=FMath::Max(0.,SoundWait-Dt);
 BeamOff=In.bBeam?0.:BeamOff+Dt;
 const bool bLampNow=In.bBeam&&LampWait<=0&&In.BodyDist>TooNear;
 // A lamp put out, not just swung away: it does not stay on it for long.
 const double Forget=In.bLampOn?1.2:.4;
 switch(Do)
 {
 case EDo::None:
 case EDo::Turn:
  if(bLampNow){Do=EDo::Watch;T=0;Look=In.Lamp;bWillBack=Roll<.65f;break;}
  if(Do==EDo::Turn)
  {
   if(T>=(In.bLampOn?TurnSeconds:TurnSeconds*.6)||In.BodyDist<TooNear)Let(0,6);
   break;
  }
  // On top of them, a noise stops nothing either: it is what they are making as it reaches them.
  if(In.bNoise&&SoundWait<=0&&In.BodyDist>TooNear){Do=EDo::Turn;T=0;Look=In.Sound;bDidTurn=true;}
  break;
 case EDo::Watch:
  if(In.bBeam)Look=In.Lamp;
  if(BeamOff>Forget||In.BodyDist<TooNear){Let(4,0);break;}
  if(T>=WatchSeconds)
  {
   Do=bWillBack?EDo::Back:EDo::Blink;T=0;
   // A lamp already out, or one that only dips under calm flashes, did not go out because of it.
   if(bWillBack)bDidBack=true;else bDidBlink|=In.bLampOn&&!In.bCalm;
  }
  break;
 case EDo::Back:
  if(In.bBeam)Look=In.Lamp;
  if(T>=BackSeconds||BeamOff>Forget||In.BodyDist<TooNear)Let(8,0);
  break;
 case EDo::Blink:
  if(T>=BlinkSeconds)Let(10,0);
  break;
 }
}

bool FS47VisitAttention::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s visit attention: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 const auto Run=[](FS47VisitAttention& A,const FIn& In,double Seconds,float Roll){for(double S=0;S<Seconds;S+=.05)A.Step(.05,In,Roll);};
 FIn Lit;Lit.bBeam=true;Lit.bLampOn=true;Lit.BodyDist=15;Lit.Lamp=FVector(0,1.6,15);
 FS47VisitAttention First;First.Step(.05,Lit,.1f);
 Check(First.Doing()==EDo::Watch&&First.LookAt().Equals(Lit.Lamp)&&First.BackSpeed()==0,TEXT("a lamp on it stops it and it looks back along the beam"));
 Run(First,Lit,WatchSeconds+.1,.1f);
 Check(First.Doing()==EDo::Back&&First.BackSpeed()>0&&First.BackSpeed()<1&&First.bDidBack,TEXT("then it draws back, slowly"));
 Run(First,Lit,BackSeconds+.1,.1f);
 Check(!First.Holding(),TEXT("the moment ends even with the lamp still on it"));
 Run(First,Lit,7.,.1f);
 Check(!First.Holding(),TEXT("and a lamp held on it cannot pin it down: there is a pause before the next"));
 Run(First,Lit,1.5,.1f);
 Check(First.Holding(),TEXT("after the pause the lamp counts again"));
 FS47VisitAttention Second;Second.Step(.05,Lit,.9f);Run(Second,Lit,WatchSeconds+.1,.9f);
 Check(Second.Doing()==EDo::Blink&&Second.BlinkLevel(false)==1.&&Second.BlinkLevel(true)<.5&&Second.BackSpeed()==0,TEXT("some it puts out for a moment instead; calm flashes keep that mild"));
 Run(Second,Lit,BlinkSeconds+.1,.9f);
 Check(!Second.Holding()&&Second.BlinkLevel(false)==0,TEXT("and it comes back on its own"));
 FIn Dark=Lit;Dark.bBeam=false;Dark.bLampOn=false;
 FS47VisitAttention PutOut;PutOut.Step(.05,Lit,.1f);Run(PutOut,Dark,.6,.1f);
 FIn Away=Dark;Away.bLampOn=true;
 FS47VisitAttention Swung;Swung.Step(.05,Lit,.1f);Run(Swung,Away,.6,.1f);
 Check(!PutOut.Holding()&&Swung.Holding(),TEXT("a lamp put out loses it sooner than one swung away"));
 FIn Near=Lit;Near.BodyDist=1.5;
 FS47VisitAttention OnTop;OnTop.Step(.05,Near,.1f);
 Check(!OnTop.Holding(),TEXT("once it is on top of them a lamp stops nothing"));
 FIn Noise;Noise.bNoise=true;Noise.Sound=FVector(5,0,5);
 FS47VisitAttention Listener;Listener.Step(.05,Noise,0);
 Check(Listener.Doing()==EDo::Turn&&Listener.Heard()&&Listener.LookAt().Equals(Noise.Sound)&&Listener.BackSpeed()==0,TEXT("a noise turns it to where the noise was"));
 FIn Still;Run(Listener,Still,TurnSeconds+.1,0);
 Check(!Listener.Holding(),TEXT("and then it goes on"));
 Listener.Step(.05,Noise,0);
 Check(!Listener.Holding(),TEXT("the next noise right after it does not hold it again"));
 Listener.Step(.05,Lit,.1f);
 Check(Listener.Doing()==EDo::Watch,TEXT("a lamp outranks a noise"));
 FIn MildLit=Lit;MildLit.bCalm=true;
 FS47VisitAttention Mild;Mild.Step(.05,MildLit,.9f);Run(Mild,MildLit,WatchSeconds+.1,.9f);
 Check(Mild.Doing()==EDo::Blink&&!Mild.bDidBlink,TEXT("under calm flashes the lamp never goes out, so the record does not say it did"));
 FS47VisitAttention Own;Own.Step(.05,Lit,.9f);Run(Own,Lit,WatchSeconds-.2,.9f);Run(Own,Dark,.3,.9f);
 Check(Own.Doing()==EDo::Blink&&!Own.bDidBlink,TEXT("a lamp the player put out themselves is not one it put out"));
 FIn Close=Noise;Close.BodyDist=1.5;
 FS47VisitAttention Reached;Reached.Step(.05,Close,0);
 Check(!Reached.Holding(),TEXT("a noise does not stop what is already on top of them"));
 FS47VisitAttention Turning;Turning.Step(.05,Noise,0);FIn CloseQuiet;CloseQuiet.BodyDist=1.5;Turning.Step(.05,CloseQuiet,0);
 Check(!Turning.Holding(),TEXT("and a player who walks up to it while it looks away is not protected by that"));
 FS47VisitAttention Calm;Run(Calm,Still,30.,0);
 Check(!Calm.Holding()&&!Calm.bDidBack&&!Calm.bDidBlink&&!Calm.bDidTurn,TEXT("no lamp and no noise: nothing happens and nothing is recorded"));
 return OK;
}

#include "S47Audio.h"
#include "S47Rx.h"

// Instrument feedback, not a recording or evidence: never adds the anomaly's 4/7 pulse.
// Use exactly the profile/quality shown by the receiver, including array pointing.
void US47Audio::MonitorReceiver(const FS47Rx& Rx, const FS47Profile& Profile)
{
 const double Quality=Rx.Quality(Profile);
 if(!Ctx || !Rx.bPowered || !FMath::IsFinite(Quality) || !FMath::IsFinite(Rx.Gain) || !FMath::IsFinite(Rx.Bandwidth) || !FMath::IsFinite(Rx.Frequency))
 {ReceiverMonitorOff();return;}
 StartCarrier();
 bReceiverMonitor=true;
 const double G=FMath::Clamp(Rx.Gain/100.,0.,1.);
 const double B=FMath::Clamp(Rx.Bandwidth/100.,0.,1.);
 const double Q=FMath::Clamp(Quality,0.,1.);
 const double Offset=FMath::Clamp((Rx.Frequency-Profile.F)/FMath::Max(Profile.FTol*8.,.05),-1.,1.);
 // Gain/BW remain audible even within the accepted quality window. Narrowing the
 // receiver removes noise; wrong gain/pointing/frequency makes the carrier recede.
 SetCarrier(.085*Q*Q*(.2+.8*G),.3*Offset,(.008+.045*(1.-Q))*(.25+.75*G)*(.25+.75*FMath::Sqrt(B)));
 FS47AudioContext::FBatch Batch(*Ctx);
 const double T=Ctx->CurrentTime();
 Carrier.Filter->Frequency.SetTargetAtTime(650.+2000.*FMath::Sqrt(B),T,.08);
 Carrier.Filter->Q.SetTargetAtTime(.45+1.1*(1.-B),T,.08);
}

void US47Audio::ReceiverMonitorOff()
{
 // The carrier is shared with Story. Only silence it when this monitor owns it.
 if(!bReceiverMonitor)return;
 bReceiverMonitor=false;
 SetCarrier(0,0,0);
 if(Ctx && Carrier.Filter)
 {
  FS47AudioContext::FBatch Batch(*Ctx);
  Carrier.Filter->Frequency.SetTargetAtTime(1800,Ctx->CurrentTime(),.08);
  Carrier.Filter->Q.SetTargetAtTime(.6,Ctx->CurrentTime(),.08);
 }
}

bool US47Audio::ReceiverMonitorSelfTest(TArray<FString>& Lines)
{
 bool Good=true;
 auto Check=[&](bool Ok,const TCHAR* Name){Good&=Ok;Lines.Add(FString::Printf(TEXT("%s receiver monitor: %s"),Ok?TEXT("PASS"):TEXT("FAIL"),Name));};
 auto* A=NewObject<US47Audio>();A->Unlock(false,false);
 auto Render=[&]()
 {
  const int32 N=48000;TArray<float> PCM;PCM.SetNumZeroed(N*2);A->Ctx->Render(PCM.GetData(),N,2);
  double Energy=0;bool Finite=true;for(int32 I=N;I<PCM.Num();++I){Finite&=FMath::IsFinite(PCM[I])&&FMath::Abs(PCM[I])<1.;Energy+=double(PCM[I])*PCM[I];}
  Check(Finite,TEXT("rendered PCM is finite and below clipping"));return FMath::Sqrt(Energy/N);
 };
 const FS47Profile& P=S47Signal::Profiles()[0];FS47Rx Rx;Rx.bPowered=true;
 Rx.Frequency=P.F;Rx.Gain=P.Gain[0];Rx.Bandwidth=P.Bw[0];Rx.Azimuth=(P.Az[0]+P.Az[1])*.5;
 A->MonitorReceiver(Rx,P);const double Locked=Render();
 Check(A->ReceiverMonitorOn()&&Locked>.005,TEXT("locked receiver renders an audible carrier"));
 Rx.Gain=P.Gain[1];A->MonitorReceiver(Rx,P);const double Higher=Render();
 Check(Higher>Locked*1.1,TEXT("gain changes audible level inside the accepted window"));
 const double Narrow=A->Carrier.Filter->Frequency.Value();Rx.Bandwidth=P.Bw[1];A->MonitorReceiver(Rx,P);Render();
 Check(A->Carrier.Filter->Frequency.Value()>Narrow+200,TEXT("bandwidth changes the rendered noise filter within the accepted window"));
 Rx.Frequency=P.F+.4;A->MonitorReceiver(Rx,P);const double Searching=Render();
 Check(Searching>.0001&&Searching<Locked*.8,TEXT("off-frequency search renders static and loses the strong carrier"));
 Check(A->Carrier.Osc->Frequency.Value()>900,TEXT("frequency sweep moves the carrier pitch"));
 Rx.bPowered=false;A->MonitorReceiver(Rx,P);const double Silent=Render();
 Check(!A->ReceiverMonitorOn()&&Silent<1e-5,TEXT("power loss releases the monitor and fades PCM to silence"));
 Rx.bPowered=true;Rx.Frequency=P.F;A->MonitorReceiver(Rx,P);Render();A->ReceiverMonitorOff();
 Check(Render()<1e-5&&!A->ReceiverMonitorOn(),TEXT("leaving the receiver fades it to silence"));
 A->SetCarrier(.04,0,0);A->ReceiverMonitorOff();
 Check(Render()>.005,TEXT("inactive monitor does not mute the Story-owned carrier"));
 return Good;
}

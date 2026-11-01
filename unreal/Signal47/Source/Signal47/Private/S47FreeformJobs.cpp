#include "S47FreeformJobs.h"
#include "S47Observing.h"
#include "S47Json.h"

namespace
{
 // Retain the four existing measured-source identities and their recorded audio.
 // Small receiver/pointing variants stay inside each authored acquisition window.
 // These are fictional observing cards, not an astronomical ephemeris solver.
 struct FSourceRule
 {
  double AzOffsets[3],ElOffsets[3],FrequencyOffsets[3],Seconds[3];
  int32 StartMinute,MinuteSpread;
  FS47FreeformJobs::EControl Control;
 };
 const FSourceRule Rules[4]={
  {{-2,0,2},{-1,0,1},{-.005,0,.005},{12,14,16},19*60,20,FS47FreeformJobs::EControl::PointingCross},
  {{-2,0,2},{-1,0,1},{-.005,0,.005},{18,18,18},19*60+50,25,FS47FreeformJobs::EControl::PulsarTiming},
  {{-2,0,2},{-1,0,1},{-.003,0,.003},{18,20,22},20*60+45,25,FS47FreeformJobs::EControl::InternalReference},
  {{-2,0,2},{-1,0,1},{-.005,0,.005},{14,16,18},22*60+5,25,FS47FreeformJobs::EControl::InternalReference}
 };
 // Explicit integer mixing: no GetTypeHash implementation/order dependency.
 uint32 Mix(uint32 V)
 {V^=V>>16;V*=0x7feb352du;V^=V>>15;V*=0x846ca68bu;return V^(V>>16);}
 FRandomStream Stream(int32 Seed,int32 Day,int32 Slot,int32 Source)
 {return FRandomStream(int32(Mix(uint32(Seed)^Mix(uint32(Day))^Mix(uint32(Slot)+0x4a4f4253u)^Mix(uint32(Source)+0x5341524fu))));}
 bool Same(const FS47FreeformJobs::FCard& A,const FS47FreeformJobs::FCard& B)
 {
  return A.Day==B.Day&&A.Slot==B.Slot&&A.Source==B.Source&&A.El==B.El&&A.Seconds==B.Seconds&&A.SuggestedClock==B.SuggestedClock&&A.Control==B.Control&&
   A.Receiver.Card==B.Receiver.Card&&A.Receiver.F==B.Receiver.F&&A.Receiver.Az[0]==B.Receiver.Az[0]&&A.Receiver.Az[1]==B.Receiver.Az[1]&&
   A.Receiver.Gain[0]==B.Receiver.Gain[0]&&A.Receiver.Gain[1]==B.Receiver.Gain[1]&&A.Receiver.Bw[0]==B.Receiver.Bw[0]&&A.Receiver.Bw[1]==B.Receiver.Bw[1];
 }
}
FString FS47FreeformJobs::ControlText(EControl Control)
{
 switch(Control)
 {
 case EControl::PointingCross:return TEXT("Five-point physical pointing cross; preserve its measured chart.");
 case EControl::PulsarTiming:return TEXT("Compare retained timing record to the 1.18791 s reference; if extra ticks occur, use the receiver-rack internal control.");
 default:return TEXT("Retain the original measurement; use the receiver-rack internal reference if a discrepancy occurs.");
 }
}
FS47FreeformJobs::FCard FS47FreeformJobs::Make(int32 Seed,int32 Day,int32 Slot,int32 Source)
{
 FCard Out;Out.Day=FMath::Max(1,Day);Out.Slot=FMath::Clamp(Slot,0,DailyCount-1);
 auto Choice=Stream(Seed,Out.Day,Out.Slot,-1);
 Out.Source=Out.Slot<2?Out.Slot:(Source==2||Source==3?Source:Choice.RandRange(2,3));
 const auto& Run=FS47Observing::Runs()[Out.Source];const auto& Rule=Rules[Out.Source];
 auto Random=Stream(Seed,Out.Day,Out.Slot,Out.Source);Out.Receiver=Run.P;
 auto& P=Out.Receiver;
 const double Az=(P.Az[0]+P.Az[1])*.5+Rule.AzOffsets[Random.RandRange(0,2)];
 P.Az[0]=Az-1.5;P.Az[1]=Az+1.5;
 Out.El=Run.El+Rule.ElOffsets[Random.RandRange(0,2)];
 P.F+=Rule.FrequencyOffsets[Random.RandRange(0,2)];
 // Two legal tuning windows per source alter the receiver operation, while all
 // midpoint knob settings remain on integer gain/bandwidth and .001 MHz steps.
 const double G0=P.Gain[0],G1=P.Gain[1],B0=P.Bw[0],B1=P.Bw[1];
 if(Random.RandRange(0,1)==0)P.Gain[1]=FMath::FloorToDouble((G0+G1)*.5);
 else P.Gain[0]=FMath::CeilToDouble((G0+G1)*.5);
 if(Random.RandRange(0,1)==0)P.Bw[1]=FMath::FloorToDouble((B0+B1)*.5);
 else P.Bw[0]=FMath::CeilToDouble((B0+B1)*.5);
 Out.Seconds=Rule.Seconds[Random.RandRange(0,2)];Out.Control=Rule.Control;
 const int32 Minute=Rule.StartMinute+Random.RandRange(-Rule.MinuteSpread/5,Rule.MinuteSpread/5)*5;
 Out.SuggestedClock=Minute*60.;
 P.Stage=FString::Printf(TEXT("FREEFORM D%02d / JOB %d"),Out.Day,Out.Slot+1);P.Action=TEXT("START ACQUISITION");
 P.Card=FString::Printf(TEXT("SARO / D%02d OBSERVATION %d / %s\nSUGGESTED %02d:%02d / FLEXIBLE ORDER, NO EXPIRY\nF %.3f MHz / AZ %.1f / EL %.1f\nGAIN %.0f-%.0f / BW %.0f-%.0f kHz\nINTEGRATION %.0f s\n%s"),Out.Day,Out.Slot+1,Run.Source,Minute/60,Minute%60,P.F,Az,Out.El,P.Gain[0],P.Gain[1],P.Bw[0],P.Bw[1],Out.Seconds,*ControlText(Out.Control));
 P.Note=TEXT("Observation card retains its original settings and source identity. Normal measurements remain valid; an anomaly requires independent control before interpretation.");
 return Out;
}
TArray<FS47FreeformJobs::FCard> FS47FreeformJobs::Generate(int32 Seed,int32 Day)
 {TArray<FCard> Out;for(int32 Slot=0;Slot<DailyCount;++Slot)Out.Add(Make(Seed,Day,Slot));return Out;}
bool FS47FreeformJobs::Valid(const FCard& C)
{
 if(C.Day<1||C.Slot<0||C.Slot>=DailyCount||C.Source<0||C.Source>3||(C.Slot<2&&C.Source!=C.Slot)||(C.Slot==2&&C.Source<2))return false;
 const auto& P=C.Receiver;const auto& Base=FS47Observing::Runs()[C.Source].P;
 if(!FMath::IsFinite(P.F)||P.F<S47Signal::FMin||P.F>S47Signal::FMax||FMath::Abs(P.F-Base.F)>Base.FTol||P.FTol!=Base.FTol)return false;
 if(!FMath::IsFinite(C.El)||C.El<5||C.El>85||!FMath::IsFinite(C.Seconds)||C.Seconds<12||C.Seconds>24||(C.Source==1&&C.Seconds!=18))return false;
 if(!FMath::IsFinite(C.SuggestedClock)||C.SuggestedClock<18*3600||C.SuggestedClock>=23*3600||C.Control!=Rules[C.Source].Control)return false;
 for(double V:{P.FTol,P.Az[0],P.Az[1],P.Gain[0],P.Gain[1],P.Bw[0],P.Bw[1]})if(!FMath::IsFinite(V))return false;
 if(P.Az[0]<Base.Az[0]||P.Az[1]>Base.Az[1]||P.Az[0]>=P.Az[1]||P.Gain[0]<Base.Gain[0]||P.Gain[1]>Base.Gain[1]||P.Gain[0]>=P.Gain[1]||P.Bw[0]<Base.Bw[0]||P.Bw[1]>Base.Bw[1]||P.Bw[0]>=P.Bw[1])return false;
 FS47Rx Rx;Rx.Frequency=P.F;Rx.Gain=FMath::RoundToDouble((P.Gain[0]+P.Gain[1])*.5);Rx.Bandwidth=FMath::RoundToDouble((P.Bw[0]+P.Bw[1])*.5);Rx.Azimuth=(P.Az[0]+P.Az[1])*.5;
 return Rx.Pass(P)&&!P.Card.IsEmpty();
}
bool FS47FreeformJobs::SelfTest(TArray<FString>& Results)
{
 bool Good=true,bHI=false,bVirgo=false,bDifferent=false;int32 Cards=0;
 for(int32 Seed:{0,1,47,-1,MAX_int32})for(int32 Day=1;Day<=32;++Day)
 {
  const auto A=Generate(Seed,Day),B=Generate(Seed,Day);Good&=A.Num()==DailyCount&&B.Num()==DailyCount;
  auto Saved=S47J::New();S47J::Set(Saved,TEXT("seed"),Seed);S47J::Set(Saved,TEXT("day"),Day);
  const auto Reload=S47J::Read(S47J::Write(Saved));
  const auto Replayed=Generate(S47J::Int(Reload,TEXT("seed")),S47J::Int(Reload,TEXT("day")));
  for(int32 I=0;I<A.Num();++I)
  {
   ++Cards;Good&=Valid(A[I])&&Same(A[I],B[I])&&Same(A[I],Replayed[I]);
   const auto& P=A[I].Receiver;FS47Rx Rx;Rx.Frequency=P.F;Rx.Gain=FMath::RoundToDouble((P.Gain[0]+P.Gain[1])*.5);Rx.Bandwidth=FMath::RoundToDouble((P.Bw[0]+P.Bw[1])*.5);Rx.Azimuth=(P.Az[0]+P.Az[1])*.5;
   Good&=Rx.Pass(P);Rx.Frequency=P.F+.15;Good&=!Rx.Pass(P);
   Rx.Frequency=P.F;Rx.Azimuth=P.Az[1]+20;Good&=!Rx.Pass(P);
   auto Invalid=A[I];Invalid.Receiver.F=S47Signal::FMax+1;Good&=!Valid(Invalid);
  }
  if(A.Num()==DailyCount){bHI|=A[2].Source==2;bVirgo|=A[2].Source==3;const auto Next=Make(Seed,Day+1,0);bDifferent|=A[0].Receiver.F!=Next.Receiver.F||A[0].Receiver.Az[0]!=Next.Receiver.Az[0]||A[0].Seconds!=Next.Seconds;}
  for(int32 Source:{2,3}){const auto Override=Make(Seed,Day,2,Source);Good&=Valid(Override)&&Same(Override,Make(Seed,Day,2,Source))&&Override.Source==Source;}
 }
 Good&=bHI&&bVirgo&&bDifferent;
 Results.Add(FString::Printf(TEXT("%s / %d deterministic legal daily cards, JSON seed/day replay, measured off-frequency/off-azimuth rejection, both survey choices, stable overrides, source-correct controls, pulsar recording retained"),Good?TEXT("PASS"):TEXT("FAIL"),Cards));
 return Good;
}

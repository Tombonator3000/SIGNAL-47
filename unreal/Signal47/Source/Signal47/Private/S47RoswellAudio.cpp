#include "S47RoswellAudio.h"
#include "S47Audio.h"
#include "S47Three.h"
#include "Misc/FileHelper.h"

namespace
{
 // Original, dry small-foot Foley, not pitched adult boots. Each variant has
 // a toe pad, delayed grit/sole movement and (only for roof metal) a short ring.
 TArray<TSharedPtr<FS47AudioBuffer>> MakeBank()
 {
  TArray<TSharedPtr<FS47AudioBuffer>> Bank;
  constexpr int32 Rate=48000;
  for (int32 Metal=0;Metal<2;++Metal) for (int32 V=0;V<6;++V)
  {
   const int32 Count=FMath::RoundToInt(Rate*(Metal?.28:.19));
   auto B=MakeShared<FS47AudioBuffer>(1,Count,Rate);
   FRandomStream Noise(47031+V*193+Metal*901);
   double Smooth=0., Slow=0.;
   for (int32 I=0;I<Count;++I)
   {
    const double T=double(I)/Rate, N=Noise.FRandRange(-1.f,1.f);
    Smooth+=(N-Smooth)*.19; Slow+=(N-Slow)*.025;
    const double Attack=FMath::Min(1.,T/.0035);
    const double Pad=FMath::Sin(2.*PI*(130.+V*9.)*T)*FMath::Exp(-T*64.);
    const double Tip=(N-Smooth)*FMath::Exp(-T*(Metal?130.:170.));
    const double ScuffStart=.025+V*.003;
    const double ScuffT=FMath::Max(0.,T-ScuffStart);
    const double Scuff=T>ScuffStart?(Smooth-Slow)*FMath::Sin(FMath::Min(1.,ScuffT/.016)*PI*.5)*FMath::Exp(-ScuffT*35.):0.;
    double Value=.25*Pad+.21*Tip+.7*Scuff;
    if (Metal)
    {
     const double Hz=390.+V*23.;
     Value+=.15*FMath::Sin(2.*PI*Hz*T)*FMath::Exp(-T*20.)
      +.065*FMath::Sin(2.*PI*Hz*1.731*T)*FMath::Exp(-T*32.);
    }
    // The tail reaches zero to avoid a sample edge clicking after the scuff.
    const double Tail=FMath::Clamp((double(Count-I)/Rate)/.012,0.,1.);
    B->GetChannelData(0)[I]=float(Value*Attack*Tail);
   }
   Bank.Add(B);
  }
  return Bank;
 }

 TSharedPtr<FS47BufferSourceNode> PlayContact(const TSharedRef<FS47AudioContext>& C,
  const TSharedPtr<FS47AudioNode>& Out,const TSharedPtr<FS47AudioBuffer>& Buffer,
  const FVector& AtThree,const FS47RoswellFoley& Surface,double Energy,int32 Variant)
 {
  FS47AudioContext::FBatch Batch(*C);
  auto Source=C->CreateBufferSource(); Source->Buffer=Buffer;
  Source->PlaybackRate.SetValue(.97+Variant*.017+(Surface.Crouch>.5?.035:0.));
  auto Gain=C->CreateGain();
  const double Quiet=FMath::Lerp(1.,.48,FMath::Clamp(Surface.Crouch,0.,1.));
  Gain->Gain.SetValue((.45+Energy*.42)*Quiet*(Surface.bOccluded?.65:1.));
  auto HP=C->CreateBiquadFilter(); HP->SetType(TEXT("highpass")); HP->Frequency.SetValue(Surface.bRoof?65.:100.);
  auto LP=C->CreateBiquadFilter(); LP->SetType(TEXT("lowpass"));
  LP->Frequency.SetValue(Surface.bOccluded?(Surface.bRoof?1250.:1750.):(Surface.Crouch>.5?4800.:6800.));
  auto Pan=C->CreatePanner(); Pan->PanningModel=TEXT("HRTF"); Pan->DistanceModel=ES47DistanceModel::Inverse;
  Pan->RefDistance=1.1; Pan->RolloffFactor=.72; Pan->MaxDistance=40.;
  Pan->SetPosition(AtThree.X,AtThree.Y,AtThree.Z);
  Source->Connect(HP); HP->Connect(LP); LP->Connect(Gain); Gain->Connect(Pan); Pan->Connect(Out);
  // No callbacks capture the owning actor. The graph and active source retain
  // one another only until the bounded sample ends; Reset can stop it safely.
  Source->Start(C->CurrentTime());
  return Source;
 }
}

void FS47RoswellAudio::Step(US47Audio* Audio,const FS47RoswellContacts& Contacts,const FS47RoswellFoley& Surface)
{
 if (Surface.bHeld || !Audio || !Audio->bReady || !Audio->Ctx || !Audio->SfxBus)
 { Stop(); return; }
 if (Audio->Ctx->State()!=TEXT("running")) { Stop(); return; }
 Sources.RemoveAll([](const TSharedPtr<FS47BufferSourceNode>& S){ return !S || S->Ended(); });
 if (!Contacts.bLeft && !Contacts.bRight) return;
 if (Bank.IsEmpty())
 {
  Bank=MakeBank();
  Pick=GetTypeHash(Contacts.AtUE[Contacts.bLeft?0:1])^47031u;
 }
 for (int32 Side=0;Side<2;++Side)
 {
  if (!(Side==0?Contacts.bLeft:Contacts.bRight)) continue;
  // Stable per-instance variation, avoiding adjacent repeats. This changes
  // timbre only; the controller's real foot contact decides when sound occurs.
  Pick=Pick*1664525u+1013904223u;
  int32 V=int32((Pick>>16)%6); if (V==Last) V=(V+1)%6; Last=V;
  const bool Ring=Surface.bRoof && Surface.bMetal;
  ++PlayedCount;
  Sources.Add(PlayContact(Audio->Ctx.ToSharedRef(),Audio->SfxBus,Bank[V+(Ring?6:0)],
   S47Three::ToThree(Contacts.AtUE[Side]),Surface,FMath::Clamp(Contacts.Energy,0.,1.),V));
 }
}

void FS47RoswellAudio::Stop()
{
 for (auto& Source:Sources) if (Source && !Source->Ended()) Source->Stop();
 Sources.Reset();
}

void FS47RoswellAudio::Reset() { Stop(); Bank.Reset(); Pick=0; Last=-1; PlayedCount=0; }

bool FS47RoswellAudio::RenderTour(const FString& Path)
{
 constexpr int32 Rate=48000, Seconds=16, Chunk=800;
 auto C=FS47AudioContext::Make(Rate);
 C->Listener->PositionY.SetValue(1.65);
 C->Listener->ForwardZ.SetValue(-1.); C->Listener->UpY.SetValue(1.);
 const auto Buffers=MakeBank();
 TArray<TSharedPtr<FS47BufferSourceNode>> Active;
 TArray<float> Samples; Samples.SetNumZeroed(Rate*Seconds*2);
 double Next=.4; int32 Count=0;
 for (int32 F=0;F<Rate*Seconds;F+=Chunk)
 {
  const double T=double(F)/Rate;
  // 0-4s: cautious lateral steps; 5-8s: walk; 9-11s: fast retreat;
  // 12-16s: roof taps crossing overhead through the actual occlusion filter.
  const bool QuietGap=(T>=4. && T<5.) || (T>=8. && T<9.) || (T>=11. && T<12.);
  if (T>=Next)
  {
   Next=T+(T<4.?.69:(T<8.?.39:(T<11.?.23:.56)));
   if (!QuietGap)
   {
    FS47RoswellFoley S; S.Crouch=T<4.?.8:0.; S.bRoof=T>=12.; S.bMetal=S.bRoof; S.bOccluded=S.bRoof;
    const FVector At=S.bRoof?FVector((T-14.)*1.2,3.,-1.):FVector((T<4.?T-2.:1.),.09,-2.-FMath::Max(0.,T-9.)*.7);
    const int32 V=Count++%6;
    Active.Add(PlayContact(C,C->Destination,Buffers[V+(S.bMetal?6:0)],At,S,T<4.?.3:(T<8.?.5:.9),V));
   }
  }
  C->Render(Samples.GetData()+F*2,Chunk,2); C->Tick(double(Chunk)/Rate);
  Active.RemoveAll([](const TSharedPtr<FS47BufferSourceNode>& S){ return S->Ended(); });
 }
 TArray<uint8> Wav;
 const auto U16=[&](uint16 V){ Wav.Add(uint8(V)); Wav.Add(uint8(V>>8)); };
 const auto U32=[&](uint32 V){ Wav.Add(uint8(V)); Wav.Add(uint8(V>>8)); Wav.Add(uint8(V>>16)); Wav.Add(uint8(V>>24)); };
 const auto Tag=[&](const ANSICHAR* S){ for (int32 I=0;I<4;++I) Wav.Add(uint8(S[I])); };
 const uint32 Bytes=Samples.Num()*2;
 Tag("RIFF"); U32(36+Bytes); Tag("WAVE"); Tag("fmt "); U32(16); U16(1); U16(2);
 U32(Rate); U32(Rate*4); U16(4); U16(16); Tag("data"); U32(Bytes);
 for (float V:Samples) U16(uint16(int16(FMath::RoundToInt(FMath::Clamp(FMath::IsFinite(V)?V:0.f,-1.f,1.f)*32767.f))));
 C->Close();
 return FFileHelper::SaveArrayToFile(Wav,*Path);
}

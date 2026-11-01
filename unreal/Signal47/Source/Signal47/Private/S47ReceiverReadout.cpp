#include "S47ReceiverReadout.h"
#include "SS47Widgets.h"
#include "Brushes/SlateColorBrush.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
 const FLinearColor Green(.42f,.94f,.55f),Amber(1.f,.67f,.28f),Dim(.15f,.36f,.21f);
 double Unit(double V){return FMath::IsFinite(V)?FMath::Clamp(V,0.,1.):0.;}
 using FData=S47ReceiverReadout::FData;
 double Quality(const FData& D)
 {
  const auto R=D.GetReceiver?D.GetReceiver():FS47Rx();if(!R.bPowered)return 0;
  return Unit(D.GetQuality?D.GetQuality():(D.GetProfile?R.Quality(D.GetProfile()):R.Quality()));
 }
 class SReceiverTrace:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SReceiverTrace){} SLATE_ARGUMENT(TSharedPtr<FData>,Data) SLATE_END_ARGS()
  void Construct(const FArguments& A){Data=A._Data;ForceVolatile(true);}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(640,132);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool)const override
  {
   if(!Data)return L;const FData& D=*Data;
   const FS47Rx R=D.GetReceiver?D.GetReceiver():FS47Rx();const FS47Profile P=D.GetProfile?D.GetProfile():R.Profile();
   const FVector2f Size(G.GetLocalSize());const float W=Size.X,H=Size.Y;
   static const FSlateColorBrush White(FLinearColor::White);
   auto Box=[&](float X,float Y,float Width,float Height,FLinearColor C){FSlateDrawElement::MakeBox(Out,L,G.ToPaintGeometry(FVector2f(Width,Height),FSlateLayoutTransform(FVector2f(X,Y))),&White,ESlateDrawEffect::None,C);};
   auto Line=[&](const TArray<FVector2f>& Points,FLinearColor C,float Width=1){FSlateDrawElement::MakeLines(Out,L+1,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,C,true,Width);};
   auto Fx=[&](double F){return float((F-S47Signal::FMin)/(S47Signal::FMax-S47Signal::FMin)*W);};
   Box(0,0,W,H,FLinearColor(.002f,.009f,.004f));
   for(int32 I=1;I<8;++I)Line({{W*I/8,0},{W*I/8,H}},Dim*.6f);
   for(int32 I=1;I<5;++I)Line({{0,H*I/5},{W,H*I/5}},Dim*.6f);
   if(!R.bPowered)return L+2;
   const double T=FPlatformTime::Seconds(),Q=Quality(D);
   const float Tune=Fx(R.Frequency),Peak=Fx(P.F),Half=FMath::Max(2.f,float(R.Bandwidth/1000/(S47Signal::FMax-S47Signal::FMin)*W*.5));
   const float X0=FMath::Clamp(Tune-Half,0.f,W),X1=FMath::Clamp(Tune+Half,0.f,W);
   Box(X0,0,FMath::Max(0.f,X1-X0),H,FLinearColor(.1f,.5f,.2f,.1f));
   TArray<FVector2f> Trace;Trace.Reserve(321);
   // The Story scope's carrier/noise presentation, evaluated against this
   // actual saved job profile instead of the Story stage's reference profile.
   for(int32 I=0;I<=320;++I)
   {
    const double X=W*I/320.,Noise=(FMath::Sin(X*.17+T*4)*2+FMath::Sin(X*.71+T*7)*3+FMath::Sin(I*12.9898+T*11)*1.5)*(R.Bandwidth/35+.2)*H/220;
    const double Amp=(12+110*Q*R.Gain/100)*H/220;
    const double Y=H*.84-Noise-Amp*FMath::Exp(-FMath::Square((X-Peak)/FMath::Max(1.,7*W/512.)))-14*H/220*FMath::Exp(-FMath::Square((X-Fx(1420.406))/FMath::Max(1.,40*W/512.)));
    Trace.Add(FVector2f(X,FMath::Clamp(float(Y),2.f,H-2)));
   }
   Line(Trace,FLinearColor(.2f,.7f,.3f,.18f),4);Line(Trace,Green,1.5f);
   if(Tune>=0&&Tune<=W)Line({{Tune,0},{Tune,H}},Amber,1.2f);
   // Profile reference is distinct from the orange tuned marker.
   if(Peak>=0&&Peak<=W)Line({{Peak,H-12},{Peak,H}},Green,2);
   return L+2;
  }
 private:TSharedPtr<FData> Data;
 };
 class SReceiverMeters:public SLeafWidget
 {
 public:
  SLATE_BEGIN_ARGS(SReceiverMeters){} SLATE_ARGUMENT(TSharedPtr<FData>,Data) SLATE_END_ARGS()
  void Construct(const FArguments& A){Data=A._Data;ForceVolatile(true);}
  virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(640,12);}
  virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 L,const FWidgetStyle&,bool)const override
  {
   static const FSlateColorBrush White(FLinearColor::White);const FVector2f S(G.GetLocalSize());
   FSlateDrawElement::MakeBox(Out,L,G.ToPaintGeometry(),&White,ESlateDrawEffect::None,Dim*.35f);
   const float P=Data?Quality(*Data):0;
   FSlateDrawElement::MakeBox(Out,L+1,G.ToPaintGeometry(FVector2f(S.X*P,S.Y),FSlateLayoutTransform()),&White,ESlateDrawEffect::None,P>=.995?Green:Amber);
   return L+2;
  }
 private:TSharedPtr<FData> Data;
 };
}
TSharedRef<SWidget> S47ReceiverReadout::Build(const FData& Input)
{
 const auto D=MakeShared<FData>(Input);
 auto Text=[&](TFunction<FString()> Value,int32 Size,FLinearColor C)
 {return SNew(STextBlock).Text_Lambda([Value]{return FText::FromString(Value());}).Font(S47Style::Mono(Size)).ColorAndOpacity(C).AutoWrapText(true);};
 return SNew(SVerticalBox)
  +SVerticalBox::Slot().AutoHeight().Padding(0,3)
   [Text([D]{return FString::Printf(TEXT("RECEPTION %d %%  /  %s"),S47Signal::CarrierPercent(Quality(*D)),D->GetRecording&&D->GetRecording()?TEXT("RECORDING"):D->GetReady&&D->GetReady()?TEXT("READY / START ACQUISITION"):TEXT("NOT READY"));},26,Green)]
  +SVerticalBox::Slot().AutoHeight().Padding(0,4)[SNew(SReceiverMeters).Data(D)]
  +SVerticalBox::Slot().AutoHeight().Padding(0,4)[SNew(SReceiverTrace).Data(D)]
  +SVerticalBox::Slot().AutoHeight()
   [Text([D]{const auto R=D->GetReceiver?D->GetReceiver():FS47Rx();const auto P=D->GetProfile?D->GetProfile():R.Profile();return FString::Printf(TEXT("ORANGE: TUNED %.3f MHz   GREEN: TARGET %.3f MHz"),R.Frequency,P.F);},18,Amber)]
  +SVerticalBox::Slot().AutoHeight().Padding(0,4)
   [Text([D]{return FString::Printf(TEXT("ARRAY: %s / %.0f %% ELEMENTS AVAILABLE   RECORDING: %.0f %%"),D->GetTrackingReady&&D->GetTrackingReady()?TEXT("POINTING AGREED"):TEXT("WAITING FOR POINTING"),100*Unit(D->GetArrayQuality?D->GetArrayQuality():0.),100*Unit(D->GetProgress?D->GetProgress():0.));},20,Green)]
  +SVerticalBox::Slot().AutoHeight().Padding(0,4)
   [Text([D]{return FString(TEXT("NEXT / "))+(D->GetNextAction?D->GetNextAction():FString(TEXT("Inspect the active work card.")));},24,Amber)];
}

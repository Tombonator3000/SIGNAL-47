#include "S47AnomalyWorld.h"
#include "S47AnomalyDread.h"
#include "S47Album.h"
#include "S47Anomaly.h"
#include "S47Audio.h"
#include "S47FieldCamera.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47RoadTerrain.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	double DistanceXZ(const FVector& A, const FVector& B) { return FVector2D(A.X - B.X, A.Z - B.Z).Size(); }
	FS47Obj Part(AS47Web* W, const FS47Obj& Parent, const FString& Name, const FVector& At, const FVector& Size, uint32 Color, bool bSphere = false, float Glow = 0)
	{
		const FS47WebNode* N = W ? W->Node(Parent.I) : nullptr;
		if (!N || !N->Comp.IsValid()) return {};
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, bSphere ? TEXT("/Engine/BasicShapes/Sphere.Sphere") : TEXT("/Engine/BasicShapes/Cube.Cube"));
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
		if (!Mesh || !Base) return {};
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *Name);
		C->SetStaticMesh(Mesh); C->SetMobility(EComponentMobility::Movable);
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		C->SetCastShadow(false); C->SetupAttachment(N->Comp.Get());
		C->SetRelativeLocation(S47Three::ToUE(At)); C->SetRelativeScale3D(S47Three::DirToUE(Size));
		UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, W);
		Mat->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Color));
		Mat->SetVectorParameterValue(TEXT("Emissive"), S47Three::Hex(Color));
		Mat->SetScalarParameterValue(TEXT("EmissivePower"), Glow);
		C->SetMaterial(0, Mat); C->RegisterComponent();
		FS47Obj Result=W->ObjAt(W->AdoptNode(C, Parent.I, Name));
		// AdoptNode preserves position only. Keep authored mesh dimensions in the
		// node transform so later exposure placement/rotation cannot inflate them.
		Result.SetScale(Size);
		return Result;
	}
}

FVector FS47AnomalyWorld::World(const FVector& P) const { return Town ? Town->Group.LocalToWorld(P) : P; }
FVector FS47AnomalyWorld::ReferenceAt() const { return World(Town ? Town->PilotCutoutAt(false) + FVector(0, .62, 0) : FVector(20.31, .62, 2062.3)); }
FVector FS47AnomalyWorld::ChangedAt() const
{
 FVector P=Town?Town->PilotCutoutAt(true):FVector(17.8,0,2044);
 if(M && M->Anomaly && M->Anomaly->Data.A02Stage>=2) {P.X=16.1;P.Y=S47RoadTerrain::SurfaceY(P.X,P.Z);}
 return World(P+FVector(0,.62,0));
}
FVector FS47AnomalyWorld::N02TraceAt() const { return World(FVector(-14.84,Lamp?Town->Group.WorldToLocal(LampAt()).Y-.42:1.2,2128.2)); }
FVector FS47AnomalyWorld::PrintShadowAt() const
{
 const double Yaw=M&&M->Anomaly&&M->Anomaly->Data.A02FacingSet?M->Anomaly->Data.A02Yaw:PI/2;
 return ChangedAt()+FVector(FMath::Sin(Yaw),0,FMath::Cos(Yaw))*1.25+FVector(FMath::Cos(Yaw),0,-FMath::Sin(Yaw))*.65+FVector(0,.25,0);
}
FVector FS47AnomalyWorld::LampAt() const { return Lamp ? Lamp.WorldPosition() : World(FVector(-14.1, 1.65, 2130.4)); }
FVector FS47AnomalyWorld::StudioAt() const
{
	const FS47Obj Studio = Town ? Town->Proxy(TEXT("studio")) : FS47Obj();
	return Studio ? Studio.WorldPosition() : World(FVector(19.7, 1.4, 2036));
}
bool FS47AnomalyWorld::Here() const { return M && M->World && (M->World->Area == TEXT("diner") || M->World->Area == TEXT("road")) && !M->World->bDriving; }
bool FS47AnomalyWorld::Free() const { return M && M->UI && !M->UI->Modal() && !M->OneAtATime() && !(M->Audio && M->Audio->PhonePlaying()) && M->Story && !M->Story->IsCinematic(); }
void FS47AnomalyWorld::Toast(const FString& Text) const { if (M && M->UI) M->UI->Toast(Text, 2.6); }

void FS47AnomalyWorld::Begin()
{
	// Same-candidate graphics control only; normal play always builds the pilot.
	bDisabled = FParse::Param(FCommandLine::Get(), TEXT("S47AnomalyWorldOff"));
	if (bDisabled) return;
	if (!M || !M->TownLayer || !M->TownLayer->Town || !M->Anomaly || !M->Web || !M->Inter) return;
	Town = M->TownLayer->Town.Get();
	// Loading another case must not retain this case's short source or timer.
	StopPrinter();
	SoundLeft = SoundRetry = ApproachTime = StillTime = 0; if(PrinterPan)PrinterPan->Disconnect();PrinterPan.Reset();Printer={};
	LastPlayer=M->Player?M->Player->Pos:FVector::ZeroVector;
	if(!Dread) Dread=MakeShared<FS47AnomalyDread>(M); else Dread->Reset();
	if (!bBuilt) { InstallGeometry(); bBuilt = true; }
	if(M->Anomaly->Data.P01Heard&&!M->Anomaly->Data.P01Ended) {ApproachTime=FMath::Clamp(S47Timeline::Night(M->Story->Clock())-S47Timeline::Night(M->Anomaly->Data.P01Clock),0.,8.);SoundLeft=FMath::Max(.01,8.-ApproachTime);}
	AfterPhoto();
	PlaceFigure();
}

void FS47AnomalyWorld::InstallGeometry()
{
	AS47Web* W = M->Web;
	// The old window was a baked picture of a second alien. Replace only its display
	// surface with an empty backing; one exported cardboard object now occupies both places.
	TArray<FColor> Back; Back.Init(FColor(39, 29, 21), 4);
	for (const FString& P : W->PathsUnder(TEXT("world.town.runtime.panels.display."))) W->Obj(P).SetMap(S47Image::FromPixels(Back, 2, 2));
	const FS47Obj Figure = Town->CutoutObject();
	Part(W, Figure, TEXT("AnomalyPriceTag"), FVector(-.16, .37, -.028), FVector(.22, .13, .018), 0xa57b22, false, 0);
	Part(W, Figure, TEXT("AnomalySplitEdge"), FVector(.115, .17, -.029), FVector(.012, .15, .012), 0x24190d);
	Part(W, Figure, TEXT("AnomalySplitEdgeNick"), FVector(.12, .235, -.03), FVector(.04, .018, .014), 0xa88b60);
	if (const FS47WebNode* N = W->Node(Figure.I); N && N->Comp.IsValid())
	{
		UTextRenderComponent* Price = NewObject<UTextRenderComponent>(W, TEXT("AnomalyPriceInk"));
		Price->SetupAttachment(N->Comp.Get()); Price->SetMobility(EComponentMobility::Movable);
		Price->SetRelativeLocation(S47Three::ToUE(FVector(-.16, .35, -.041))); Price->SetRelativeRotation(FRotator(0, -90, 0));
		Price->SetHorizontalAlignment(EHTA_Center); Price->SetWorldSize(6.f);
		Price->SetTextRenderColor(FColor::Black); Price->SetText(FText::FromString(TEXT("$3.50"))); Price->RegisterComponent();
	}
	// A weak opal work lamp over a narrow pale board: head and shoulders from the
	// pavement, ordinary lamp/bracket/board when the player steps sideways. No live figure.
	// Keep the ordinary lamp separate from the existing woman on the bench.
	const double Ground = Town->FloorAt(World(FVector(-14.1, 0, 2130.4)).X, World(FVector(-14.1, 0, 2130.4)).Z) - Town->Group.WorldPosition().Y;
	Lamp = Part(W, Town->Group, TEXT("AnomalyOpalLamp"), FVector(-14.1, Ground + 1.65, 2130.4), FVector(.27, .3, .27), 0xe1cf9b, true, .45f);
	LampShade = Part(W, Town->Group, TEXT("AnomalyPaleBoard"), FVector(-14.8, Ground + .94, 2130.4), FVector(.10, 1.05, .53), 0xaaa080, false, .08f);
	Part(W, Town->Group, TEXT("AnomalyLampBracket"), FVector(-14.5, Ground + 1.65, 2130.4), FVector(.78, .045, .045), 0x393832);
	Part(W, Town->Group, TEXT("AnomalyLampWire"), FVector(-14.85, Ground + 1.92, 2130.4), FVector(.025, .6, .025), 0x292a25);
 // These are real, hidden meshes rendered only during an actual camera exposure.
 auto FilmFigure=[&](TArray<FS47Obj>& Parts,const FString& Prefix)
 {
  Parts.Add(Part(W,Town->Group,Prefix+TEXT("Body"),FVector::ZeroVector,FVector(.10,.75,.36),0x020304,true,0));
  Parts.Add(Part(W,Town->Group,Prefix+TEXT("Head"),FVector::ZeroVector,FVector(.21,.25,.21),0x020304,true,0));
  for(auto& P:Parts) P.SetVisible(false);
 };
 FilmFigure(N02Film,TEXT("AnomalyLaundryFilm")); FilmFigure(A02Film,TEXT("AnomalyStudioFilm"));
	Part(W, Town->Group, TEXT("AnomalyReferenceMark"), FVector(17.6, S47RoadTerrain::SurfaceY(17.6, 2062.3) + .025, 2062.3), FVector(.45, .025, .65), 0xb8a67b);
	M->Inter->Add({ TEXT("anomaly:figure"), Figure, [this] { return M->Anomaly->Data.A02Moved ? TEXT("Inspect the cardboard figure") : TEXT("Record the souvenir display"); }, [this] { InspectFigure(); }, 4.5,
		[this] { return FString(TEXT("A cardboard souvenir. A faded price; a split near its foot.")); }, 9 });
	M->Inter->Add({ TEXT("anomaly:lamp"), Lamp, [] { return FString(TEXT("Check the pale shape from the side")); }, [this] { InspectLamp(); }, 8,
		[] { return FString(TEXT("A pale outline under the laundromat light.")); }, 14 });
	// Extend the existing window action without stealing its proxy before the sound.
	// Earl's buzzer remains the separate, unchanged amDoor interaction.
	if (FS47Interactable* I = M->Inter->Get(TEXT("town:studio")))
	{
		const auto OriginalUse = I->Use; const auto OriginalLabel = I->Label;
		I->Use = [this, OriginalUse] { if (M->Anomaly->Data.P01Heard) InspectStudio(); else if (OriginalUse) OriginalUse(); };
		I->Label = [this, OriginalLabel] { return Here() && M->Anomaly->Data.P01Heard ? FString(TEXT("Check the closed studio facade")) : OriginalLabel ? OriginalLabel() : FString(); };
	}
	if (FS47Interactable* I = M->Inter->Get(TEXT("town:souvenir"))) I->Use = [this] { InspectFigure(); };
	// Named alias for the same follow-up in deterministic input scenarios; no extra hit surface.
	M->Inter->Add({ TEXT("anomaly:studio"), {}, {}, [this] { InspectStudio(); }, 5.5 });
}

void FS47AnomalyWorld::PlaceFigure()
{
 if(!Town||!M||!M->Anomaly)return;
 const auto& D=M->Anomaly->Data;
 if(D.bLegacyCutout) Town->ReleasePilotCutout();
 else
 {
  Town->SetPilotCutout(D.A02Moved);
  if(D.A02Moved) {Town->CutoutObject().SetPosition(Town->Group.WorldToLocal(ChangedAt())-FVector(0,.62,0)); if(D.A02FacingSet) Town->CutoutObject().SetRotY(D.A02Yaw);}
 }
}
void FS47AnomalyWorld::Reset()
{
 StopPrinter();
 if(Dread)Dread->Reset();
 SoundLeft=SoundRetry=ApproachTime=StillTime=0; if(PrinterPan)PrinterPan->Disconnect();PrinterPan.Reset();Printer={};AfterPhoto();PlaceFigure();
 if(Town&&M&&M->Story)Town->bAnomalyBaseline=S47Timeline::Night(M->Story->Clock())<0;
}
void FS47AnomalyWorld::StopPrinter()
{
 if(M&&M->Audio)M->Audio->Stop(TEXT("anomaly-printer"),.04f);
 if(InitialPrinter.Src)InitialPrinter.Src->Stop();
 if(InitialPrinter.Gain)InitialPrinter.Gain->Disconnect();InitialPrinter={};
 if(PrinterPan)PrinterPan->Disconnect();PrinterPan.Reset();Printer={};
}
bool FS47AnomalyWorld::Busy() const { return SoundLeft>0 || (Dread&&Dread->Busy()); }
void FS47AnomalyWorld::DreadStep(double Dt) { if(Dread)Dread->Step(Dt); }
bool FS47AnomalyWorld::FilmLineClear(const FVector& To) const
{
 if(!M||!M->Web||!M->Web->GetWorld()||!M->Player)return false;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47FilmTrace),true);
 FVector From=S47Three::ToUE(FS47Cam::Of(M->Player,5000).Pos),End=S47Three::ToUE(To);
 for(int32 I=0;I<24;++I)
 {
  FHitResult Hit;if(!M->Web->GetWorld()->LineTraceSingleByChannel(Hit,From,End,ECC_Visibility,Q))return true;
  auto* C=Hit.GetComponent();bool Solid=C&&C->IsVisible();
  if(Solid)if(const auto* Mat=C->GetMaterial(FMath::Max<int32>(0,Hit.ElementIndex)))
  {const auto Blend=Mat->GetBlendMode();float Opacity=1;Mat->GetScalarParameterValue(FName(TEXT("Opacity")),Opacity);if(Blend==BLEND_Additive||(Blend==BLEND_Translucent&&Opacity<.5f))Solid=false;}
  if(Solid)return false;if(C)Q.AddIgnoredComponent(C);From=Hit.ImpactPoint;
 }
 return false;
}
void FS47AnomalyWorld::BeforePhoto()
{
 bFilmN02=bFilmA02=false;
 if(bDisabled||!Here()||!M||!M->Anomaly||!M->Story||!M->FieldCamera||S47Timeline::Night(M->Story->Clock())<0||S47Timeline::Night(M->Story->Clock())>6*3600) return;
 const auto& D=M->Anomaly->Data;
 bFilmN02=D.N02Control&&ControlStand()&&M->FieldCamera->InFrame(LampAt(),.12,.88,.12,.88)&&M->FieldCamera->InFrame(N02TraceAt(),.03,.97,.03,.97)&&FilmLineClear(N02TraceAt()+FVector(0,.47,0));
 bFilmA02=D.A02Moved&&!D.bLegacyCutout&&DistanceXZ(M->Player->Pos,ChangedAt())<9&&M->FieldCamera->InFrame(ChangedAt(),.1,.9,.1,.9)&&M->FieldCamera->InFrame(PrintShadowAt(),.03,.97,.03,.97)&&FilmLineClear(PrintShadowAt()+FVector(0,.47,0));
 auto Show=[this](TArray<FS47Obj>& Parts,const FVector& At,bool On)
 {
  for(int32 I=0;I<Parts.Num();++I) {Parts[I].SetPosition(Town->Group.WorldToLocal(At)+FVector(0,I==1?.47:0,0));Parts[I].SetVisible(On);const FVector V=M->Player->Pos-At;Parts[I].SetRotY(FMath::Atan2(-V.X,-V.Z)-PI/2);}
 };
 Show(N02Film,N02TraceAt(),bFilmN02);Show(A02Film,PrintShadowAt(),bFilmA02);
}
void FS47AnomalyWorld::AfterPhoto() {for(auto& P:N02Film)P.SetVisible(false);for(auto& P:A02Film)P.SetVisible(false);}
bool FS47AnomalyWorld::MayMove(bool bReference, bool bMoved, double Clock, double Distance)
{
	return bReference && !bMoved && S47Timeline::Night(Clock) >= 0 && Distance > 45;
}
bool FS47AnomalyWorld::MayPlay(bool bPending, bool bHeard, bool bPanel, bool bFree, bool bHere, double Clock)
{
	const double Day = S47Timeline::Day(Clock);
	return bPending && !bHeard && !bPanel && bFree && bHere && Day >= 4 * 3600 && Day < 12 * 3600;
}
void FS47AnomalyWorld::Step(double Dt)
{
 if(bDisabled)return;
 if(!Town)Begin();
 if(!Town||!M->Anomaly||!M->Story||!M->Player||!M->Audio||!M->UI)return;
 auto& D=M->Anomaly->Data;const double Clock=M->Story->Clock();
 SoundRetry=FMath::Max(0.,SoundRetry-Dt);
 const FVector Position=M->Player->Pos;
 const double Moved=FVector::Distance(Position,LastPlayer);LastPlayer=Position;
 const bool Night=S47Timeline::Night(Clock)>=0;
 Town->bAnomalyBaseline=!Night||SoundLeft>0;
 auto Face=[&]{const FVector V=Position-ChangedAt();D.A02Yaw=FMath::Atan2(-V.X,-V.Z);D.A02FacingSet=true;PlaceFigure();};
 if(!D.bLegacyCutout&&Free()&&MayMove(!D.ReferencePhoto.IsEmpty()||D.bDatedReference,D.A02Moved,Clock,Here()?DistanceXZ(Position,ReferenceAt()):10000))
 {D.A02Moved=true;D.A02Stage=1;PlaceFigure();M->Anomaly->Changed();}
 if(D.A02Moved&&!D.A02FacingSet&&Here()&&Free()&&DistanceXZ(Position,ChangedAt())>9) {Face();M->Anomaly->Changed();}
 if(!Here()||M->UI->Modal()||M->Story->IsCinematic()||M->Audio->PhonePlaying()||(Dread&&Dread->ProtectedScene()))
 {StopPrinter();SoundLeft=0;return;}
 const FS47Cam Cam=FS47Cam::Of(M->Player,5000);
 auto Looking=[&](const FVector& At,double Range){return FVector::Distance(Cam.Pos,At)<Range&&FVector::DotProduct(Cam.Forward,(At-Cam.Pos).GetSafeNormal())>.94;};
 // A second displacement happens only after the first record, outside nearby view.
 const double Dist=DistanceXZ(Position,ChangedAt());
 if(!D.bLegacyCutout&&D.A02Observed&&!D.ChangedPhoto.IsEmpty()&&D.A02Stage<2&&Dist>8&&(FVector::DotProduct(Cam.Forward,(ChangedAt()-Cam.Pos).GetSafeNormal())<.2)&&Free())
 {
  D.A02Stage=2;Face();
  if(!D.A02ScrapeHeard&&Dread)D.A02ScrapeHeard=Dread->Pulse(ChangedAt(),2);
  M->Anomaly->Changed();
 }
 // The ordinary lamp remains an ordinary lamp in the evening. Only night acquires a second trace.
 const FVector Standing=Town->Group.WorldToLocal(Position);
 if(!D.N02Observed&&Standing.X>-15&&Standing.Z>2130&&Standing.Z<2135.5&&Looking(LampAt(),15)&&Free())
 {D.N02Observed=true;D.N02Clock=Clock;M->Anomaly->Changed();Toast(TEXT("Someone by the washers."));}
 if(!D.bLegacyCutout&&D.A02Moved&&!D.A02Observed&&Looking(ChangedAt(),18)&&Free())
 {D.A02Observed=true;if(Dread)Dread->Pulse(ChangedAt(),1);M->Anomaly->Changed();Toast(TEXT("It was in the window."));}
 if(Night&&!D.P01Ended&&D.P01Heard)
 {
  ApproachTime=FMath::Max(ApproachTime+Dt,S47Timeline::Night(Clock)-S47Timeline::Night(D.P01Clock));
  if(ApproachTime>=8) {StopPrinter();SoundLeft=0;D.P01Ended=true;M->Anomaly->Changed();return;}
  const bool Moving=Moved>.012&&Moved<2&&DistanceXZ(Position,StudioAt())<24;
  StillTime=Moving?0:StillTime+Dt;
  if(Moving)
  {
   const FVector V=StudioAt()-Cam.Pos;const FVector Source=Cam.Pos+V.GetSafeNormal()*FMath::Max(1.0,V.Size()*.28);
   if(!Printer) {PrinterPan=StaticCastSharedPtr<FS47PannerNode>(M->Audio->Spatial(Source,1.5f,M->Audio->SfxBus));FS47Sound O;O.DestNode=PrinterPan;O.Gain=.62f;O.Lp=2600;Printer=M->Audio->LoopSfx(TEXT("anomaly-printer"),TEXT("printer"),O);if(Printer)D.P01Approach=true;}
   if(PrinterPan)PrinterPan->SetPosition(Source.X,Source.Y,Source.Z);
  }
  else if(Printer) {StopPrinter();}
  SoundLeft=FMath::Max(.01,8.-ApproachTime);
  if((D.P01Approach&&ApproachTime>2&&StillTime>.75)||ApproachTime>8||DistanceXZ(Position,StudioAt())>35)
  {StopPrinter();SoundLeft=0;D.P01Ended=true;M->Anomaly->Changed();Toast(TEXT("[The feeding stops.]"));}
  return;
 }
 if(Night&&SoundRetry<=0&&MayPlay(D.P01Pending,D.P01Heard,M->UI->Modal(),Free(),DistanceXZ(Cam.Pos,StudioAt())<30,Clock))
 {
  if(!M->Audio)return;
  if(Dread&&!D.P01Flickered)D.P01Flickered=Dread->Pulse(StudioAt(),3);
  FS47Sound O;O.At=StudioAt();O.Gain=.4f;O.Dur=1.0f;O.Lp=2100;
  InitialPrinter=M->Audio->Sfx(TEXT("printer"),O);
  if(!InitialPrinter) {SoundRetry=8;return;}
  SoundLeft=8;ApproachTime=StillTime=0;D.P01Heard=true;D.P01Pending=false;D.P01Clock=Clock;M->Anomaly->Changed();Toast(TEXT("[Paper feeding. Inside the glass.]"));
 }
}

bool FS47AnomalyWorld::ControlStand() const
{
	if (!M || !M->Player || !Town) return false;
	const FVector P = Town->Group.WorldToLocal(M->Player->Pos);
	return P.X > -16 && P.X < -8 && P.Z > 2136 && P.Z < 2142;
}
void FS47AnomalyWorld::InspectLamp()
{
	if (!M || !M->Anomaly || !Here()) return;
	auto& D = M->Anomaly->Data;
	if (D.N02Photo.IsEmpty()) { Toast(TEXT("The light masks its outline.")); return; }
	if (!ControlStand()) { Toast(TEXT("The bracket disappears from here.")); return; }
	D.N02Control = true; M->Anomaly->Changed();
	Toast(TEXT("Just a lamp and a board."));
}
void FS47AnomalyWorld::InspectFigure()
{
	if (!M || !M->Anomaly || !Here() || !M->Player) return;
	auto& D = M->Anomaly->Data;
	if (DistanceXZ(M->Player->Pos, D.A02Moved ? ChangedAt() : ReferenceAt()) > 6) return;
	if (!D.bLegacyCutout && D.A02Moved && D.A02Compared)
	{
		D.A02Followed = true; M->Anomaly->Changed();
		Toast(TEXT("Still cardboard."));
	}
	else M->Anomaly->Open(D.A02Moved ? TEXT("a02") : TEXT("reference"));
}
void FS47AnomalyWorld::InspectStudio()
{
	if (!M || !M->Anomaly || !M->Anomaly->Data.P01Heard || !Here() || !M->Player || DistanceXZ(M->Player->Pos, StudioAt()) > 7) return;
	M->Anomaly->Data.P01Followed = true; M->Anomaly->Changed();
	Toast(TEXT("The glass is cold. Nobody inside."));
}
void FS47AnomalyWorld::TagPhoto(FS47AlbumShot& Shot) const
{
	if (bDisabled || !Town || !Here() || !M->FieldCamera || !M->Anomaly || !M->Player || Shot.Url.IsEmpty()) return;
	const auto& D = M->Anomaly->Data;
	FString Route;
	const bool bDiag = FParse::Value(FCommandLine::Get(), TEXT("S47AnomalyRoute="), Route);
	if (bDiag) UE_LOG(LogTemp, Display, TEXT("S47: anomaly target context shot=%s eye=%s local=%s lamp=%s studio=%s control=%d"), *Shot.Id,
		*FS47Cam::Of(M->Player, 5000).Pos.ToString(), *Town->Group.WorldToLocal(M->Player->Pos).ToString(), *LampAt().ToString(), *StudioAt().ToString(), D.N02Control ? 1 : 0);
	auto Tag = [&](const FString& Id, const FVector& P, double Range)
	{
		const double Distance = DistanceXZ(M->Player->Pos, P);
		const bool bInFrame = M->FieldCamera->InFrame(P, .12, .88, .12, .88);
		if (bDiag) UE_LOG(LogTemp, Display, TEXT("S47: anomaly target %s point=%s distance=%.3f range=%.3f inFrame=%d"), *Id, *P.ToString(), Distance, Range, bInFrame ? 1 : 0);
		if (Distance > Range || !bInFrame) return;
		// A centered point behind a wall is not a documented feature. Test the actual
		// exposure line; hits on the figure itself are the intended surface.
		FCollisionQueryParams Query(SCENE_QUERY_STAT(S47AnomalyEvidence), true);
		FVector Eye = S47Three::ToUE(FS47Cam::Of(M->Player, 5000).Pos);
		const FVector End = S47Three::ToUE(P);
		const FS47Obj Target = Id.StartsWith(TEXT("anomaly:n02")) ? Lamp : Town->CutoutObject();
		const FS47WebNode* Node = M->Web->Node(Target.I);
		bool bClear = false;
		// Same rendered-surface contract as FS47Album::Blocked: invisible interaction
		// boxes and sufficiently transparent materials do not occlude an exposure.
		for (int32 Tries = 0; Tries < 24; ++Tries)
		{
			FHitResult Hit;
			if (!M->Web->GetWorld()->LineTraceSingleByChannel(Hit, Eye, End, ECC_Visibility, Query)) { bClear = true; break; }
			UPrimitiveComponent* Comp = Hit.GetComponent();
			const bool bTarget = Node && Node->Comp.IsValid() && Comp && (Comp == Node->Comp.Get() || Comp->IsAttachedTo(Node->Comp.Get()));
			bool bSolid = Comp && Comp->IsVisible();
			if (bSolid)
			{
				if (const UMaterialInterface* Mat = Comp->GetMaterial(FMath::Max<int32>(0, Hit.ElementIndex)))
				{
					const EBlendMode Blend = Mat->GetBlendMode();
					float Opacity = 1.f;
					Mat->GetScalarParameterValue(FName(TEXT("Opacity")), Opacity);
					if (Blend == BLEND_Additive || (Blend == BLEND_Translucent && Opacity < .5f)) bSolid = false;
				}
			}
			if (bDiag) UE_LOG(LogTemp, Display, TEXT("S47: anomaly target %s ray hit=%s node=%d at=%s time=%.6f targetNode=%d target=%d solid=%d"), *Id,
				*GetNameSafe(Comp), M->Web->NodeOf(Comp), *S47Three::ToThree(Hit.ImpactPoint).ToString(), Hit.Time, Target.I, bTarget ? 1 : 0, bSolid ? 1 : 0);
			if (bTarget) { bClear = true; break; }
			if (bSolid) return;
			if (Comp) Query.AddIgnoredComponent(Comp);
			Eye = Hit.ImpactPoint;
		}
		if (!bClear) return;
		const auto UV = M->FieldCamera->Project(P);
		if (bDiag) UE_LOG(LogTemp, Display, TEXT("S47: anomaly target %s clear ray front=%d uv=%.4f,%.4f"), *Id, UV.bFront ? 1 : 0, UV.U, UV.V);
		if (UV.bFront)
		{
			Shot.Targets.Add({ Id, FVector2D(UV.U, UV.V) });
			FString Caption;
			if (Id == TEXT("anomaly:reference")) Caption = TEXT("The souvenir shop display, with the cardboard figure in its window.");
			else if (Id == TEXT("anomaly:changed")) Caption = TEXT("The cardboard figure beside the radio studio.");
			else if (Id == TEXT("anomaly:n02reference")) Caption = TEXT("The bright upright outline at the laundromat.");
			else if (Id == TEXT("anomaly:n02control")) Caption = TEXT("Side view: the laundromat worklamp and upright board.");
			// Only annotate this new exposure. Retain the album's actual oddities and
			// photograph bytes; documenting the figure makes no claim about its cause.
			if (!Caption.IsEmpty() && !Shot.Text.Contains(Caption)) Shot.Text = Caption + (Shot.Text.IsEmpty() ? FString() : TEXT(" ") + Shot.Text);
		}
	};
	// The detailed tag must be legible; a distant glimpse is observation, not evidence.
	const FVector FigureAt = D.A02Moved ? ChangedAt() : ReferenceAt();
	const FVector Local = Town->Group.WorldToLocal(M->Player->Pos);
	const FVector Front=D.A02FacingSet?FVector(-FMath::Sin(D.A02Yaw),0,-FMath::Cos(D.A02Yaw)):FVector(-1,0,0);
	const bool FrontView=D.A02Moved?FVector::DotProduct(Front,(M->Player->Pos-FigureAt).GetSafeNormal())>.2:Local.X<20.0;
	if (!D.bLegacyCutout && FrontView && M->FieldCamera->InFrame(FigureAt + FVector(0, .48, 0), .08, .92, .08, .92)
		&& M->FieldCamera->InFrame(FigureAt - FVector(0, .48, 0), .08, .92, .08, .92))
		Tag(D.A02Moved ? TEXT("anomaly:changed") : TEXT("anomaly:reference"), FigureAt, 9);
 if(bFilmN02) {if(!D.N02Trace&&Dread)Dread->Pulse(LampAt(),1);const auto UV=M->FieldCamera->Project(N02TraceAt());if(UV.bFront){Shot.Targets.Add({TEXT("anomaly:n02trace"),FVector2D(UV.U,UV.V)});Shot.Text+=TEXT(" A second outline, farther back.");}}
 if(bFilmA02) {const auto UV=M->FieldCamera->Project(PrintShadowAt());if(UV.bFront){Shot.Targets.Add({TEXT("anomaly:printshadow"),FVector2D(UV.U,UV.V)});Shot.Text+=TEXT(" Another upright shape behind it.");}}
 bFilmN02=bFilmA02=false;
 const FVector P = Town->Group.WorldToLocal(M->Player->Pos);
 if (D.N02Control && ControlStand()) Tag(TEXT("anomaly:n02control"), LampAt(), 11);
	else if (P.X > -15 && P.Z > 2130 && P.Z < 2135.5) Tag(TEXT("anomaly:n02reference"), LampAt(), 13);
}

bool FS47AnomalyWorld::SelfTest(TArray<FString>& Lines)
{
	bool Ok = !MayMove(true, false, -1, 100) && MayMove(true, false, 1, 100) && !MayMove(false, false, 1, 100) && !MayMove(true, true, 1, 100) && !MayMove(true, false, 1, 20);
	Ok &= MayPlay(true, false, false, true, true, 4 * 3600 + 10 * 60) && !MayPlay(true, false, false, true, true, 3 * 3600 + 59 * 60);
	Ok &= !MayPlay(true, false, true, true, true, 5 * 3600) && !MayPlay(true, false, false, false, true, 5 * 3600) && !MayPlay(true, true, false, true, true, 5 * 3600);
	Lines.Add(FString::Printf(TEXT("%s AnomalyWorld: persistent reference/move gates; closed-studio sound waits for active comparison, free scene and closed panel"), Ok ? TEXT("PASS") : TEXT("FAIL")));
	return FS47AnomalyDread::SelfTest(Lines) && Ok;
}

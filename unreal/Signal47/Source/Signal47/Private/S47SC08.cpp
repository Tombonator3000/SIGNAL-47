#include "S47SC08.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Spine.h"
#include "S47Flow.h"
#include "S47Office.h"
#include "S47SaroNight.h"
#include "S47Away.h"
#include "S47DishesFollow.h"
#include "S47DishArray.h"
#include "S47Audio.h"
#include "S47LostNights.h"
#include "S47ReelTape.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47Night.h"
#include "S47Inter.h"
#include "S47Three.h"
#include "S47Seen.h"
#include "S47UI.h"
#include "S47Kit.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Widgets/SBoxPanel.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SC08,Log,All);
namespace
{
 enum{North,Ref,A,B,SpineMid,S02Junction,S02E,S02NE,S02NW,S02SW,S02SE,S03Junction,S03W,S03NW,S03NE,S03SE,S03SW,Gate};
 double Wrap(double V){return FMath::Atan2(FMath::Sin(FMath::DegreesToRadians(V)),FMath::Cos(FMath::DegreesToRadians(V)))*180/PI;}
 double Norm(double V){return FMath::Fmod(FMath::Fmod(V,360.)+360.,360.);}
 S47J::FObj DocSave(const FS47Doc& D)
 {
  auto O=S47J::New();S47J::Set(O,TEXT("id"),D.Id);S47J::Set(O,TEXT("title"),D.Title);S47J::Set(O,TEXT("kind"),D.Kind);S47J::Set(O,TEXT("stamp"),D.Stamp);
  S47J::Set(O,TEXT("page"),D.Page);S47J::Set(O,TEXT("transcript"),D.Transcript);S47J::Set(O,TEXT("image"),D.Image);S47J::Set(O,TEXT("paper"),D.Paper);S47J::Set(O,TEXT("paperRatio"),D.PaperRatio);
  TArray<TSharedPtr<FJsonValue>> Sheets;for(const auto& S:D.Sheets){auto V=S47J::New();S47J::Set(V,TEXT("page"),S.Page);S47J::Set(V,TEXT("paper"),S.Paper);S47J::Set(V,TEXT("strip"),S.Strip);S47J::Set(V,TEXT("term"),S.Term);Sheets.Add(MakeShared<FJsonValueObject>(V));}O->SetArrayField(TEXT("sheets"),Sheets);return O;
 }
 FS47Doc DocLoad(const S47J::FObj& O)
 {
  FS47Doc D;D.Id=S47J::Str(O,TEXT("id"));D.Title=S47J::Str(O,TEXT("title"));D.Kind=S47J::Str(O,TEXT("kind"));D.Stamp=S47J::Str(O,TEXT("stamp"));D.Page=S47J::Str(O,TEXT("page"));D.Transcript=S47J::Str(O,TEXT("transcript"));D.Image=S47J::Str(O,TEXT("image"));D.Paper=S47J::Str(O,TEXT("paper"));D.PaperRatio=S47J::Num(O,TEXT("paperRatio"));
  for(const auto& V:S47J::Arr(O,TEXT("sheets")))if(V&&V->Type==EJson::Object){const auto Q=V->AsObject();D.Sheets.Add({S47J::Str(Q,TEXT("page")),S47J::Str(Q,TEXT("paper")),S47J::Str(Q,TEXT("strip")),S47J::Str(Q,TEXT("term"))});}return D;
 }
 S47J::FObj CommandLog(const AS47Story* S)
 {
  auto O=S47J::New();TArray<TSharedPtr<FJsonValue>> Rows;if(S)for(const auto& L:S->Night().Log){auto R=S47J::New();S47J::Set(R,TEXT("clock"),L.Clock);S47J::Set(R,TEXT("text"),L.Text);Rows.Add(MakeShared<FJsonValueObject>(R));}O->SetArrayField(TEXT("rows"),Rows);return O;
 }
 FString CueText(const FS47ReelTape& Tape)
 {
  FString Out;for(const auto& C:Tape.Cues)Out+=FString::Printf(TEXT("%.6f|%s|%s|%.6f|%d\n"),C.At,*C.Kind,*C.Text,C.Secs,C.I);return Out;
 }
}
FS47SC08::~FS47SC08(){Stop();}
FVector2D FS47SC08::Point(const FString& Id){return Id==TEXT("A")?FVector2D(-8,-74):Id==TEXT("B")?FVector2D(18,-74):FVector2D(-1,-74);}
FVector2D FS47SC08::Bearing(double X,double Z,double Scale,const FVector& Eye)
{
 const double Dx=Eye.X-X,Dz=Eye.Z-Z,H=FVector2D(Dx,Dz).Size();
 return FVector2D(Norm(FMath::RadiansToDegrees(FMath::Atan2(Dx,-Dz))),FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan2(Eye.Y-12.4*Scale,FMath::Max(.5,H))),S47Follow::EL_MIN,88.));
}
const TArray<FS47SC08::FNode>& FS47SC08::Nodes()
{
 static const TArray<FNode> N={{TEXT("north-trench"),{-1,-88}},{TEXT("row-one-ref"),{-1,-74}},{TEXT("A"),{-8,-74}},{TEXT("B"),{18,-74}},
  {TEXT("south-trench"),{-1,-63}},{TEXT("S02-junction"),{-1,-52}},{TEXT("S02-east"),{-10.45,-52}},{TEXT("S02-north-east"),{-10.45,-56.3}},
  {TEXT("S02-north-west"),{-20,-56.3}},{TEXT("S02-south-west"),{-20,-47.7}},{TEXT("S02-south-east"),{-10.45,-47.7}},
  {TEXT("S03-junction"),{-1,-42}},{TEXT("S03-west"),{4,-42}},{TEXT("S03-north-west"),{4,-47.5}},{TEXT("S03-north-east"),{14.8,-47.5}},
  {TEXT("S03-south-east"),{14.8,-36.8}},{TEXT("S03-south-west"),{4,-36.8}},{TEXT("walk-gate"),{-1,-22}}};return N;
}
const TArray<TPair<int32,int32>>& FS47SC08::Edges()
{
 static const TArray<TPair<int32,int32>> E={{North,Ref},{Ref,A},{Ref,B},{Ref,SpineMid},{SpineMid,S02Junction},{S02Junction,S02E},{S02E,S02NE},{S02NE,S02NW},{S02NW,S02SW},{S02SW,S02SE},{S02SE,S02E},
  {S02Junction,S03Junction},{S03Junction,S03W},{S03W,S03NW},{S03NW,S03NE},{S03NE,S03SE},{S03SE,S03SW},{S03SW,S03W},{S03Junction,Gate}};return E;
}
bool FS47SC08::CanAdoptSC07(const S47J::FObj& State)
{
 const auto C=S47J::Obj(S47J::Obj(State,TEXT("case")),TEXT("ch2"));const auto Spine=S47J::Obj(State,TEXT("spine"));
 if(S47J::Int(C,TEXT("sc07Version"))!=1||!S47J::Bool(C,TEXT("binderCompared"))||S47J::Str(C,TEXT("stage"))!=TEXT("complete")||!S47J::Bool(C,TEXT("p04"))||!S47J::Bool(C,TEXT("p05"))||!S47J::Bool(C,TEXT("called"))
  ||S47J::Int(Spine,TEXT("v"))!=1||S47J::Int(Spine,TEXT("sc04"))!=2||S47J::Int(Spine,TEXT("sc08"))!=1||!S47J::Bool(S47J::Obj(State,TEXT("office")),TEXT("departed")))return false;
 TSet<FString> Docs;for(const auto& V:S47J::Arr(C,TEXT("sources")))if(V&&V->Type==EJson::Object&&!S47J::Str(V->AsObject(),TEXT("page")).IsEmpty())Docs.Add(S47J::Str(V->AsObject(),TEXT("id")));
 return Docs.Contains(TEXT("EV_DALE_INDEX"))&&Docs.Contains(TEXT("EV_DALE_NOTE"))&&Docs.Contains(TEXT("obs_runbinder"));
}
void FS47SC08::Begin(const S47J::FObj& Saved,bool Active)
{
 Stop();bActive=Active;P=EStep::Earl;Plays=Captures=0;First.Empty();EscapedAt=0;LastTape.Empty();Samples.Reset();SourcePapers.Reset();LogBaseline.Reset();RestoredPointings.Reset();
 bRefRecorded=bIsolated=bLabelRead=bCompared=bEscaped=bVoiced=false;AppliedRestore.Reset();T=StateT=Lost=Still=HoldT=0;FadeT=-1;
 if(!Active)return;
 if(S47J::Int(Saved,TEXT("v"))==1)
 {
  P=EStep(FMath::Clamp(S47J::Int(Saved,TEXT("step")),0,int32(EStep::Done)));Plays=FMath::Max(0,S47J::Int(Saved,TEXT("plays")));Captures=FMath::Max(0,S47J::Int(Saved,TEXT("captures")));
  First=S47J::Str(Saved,TEXT("first"));EscapedAt=S47J::Num(Saved,TEXT("escapedAt"));LastTape=S47J::Str(Saved,TEXT("reel_cues"));
  bRefRecorded=S47J::Bool(Saved,TEXT("referenceRecorded"));bIsolated=S47J::Bool(Saved,TEXT("isolated"));bLabelRead=S47J::Bool(Saved,TEXT("labelRead"));bCompared=S47J::Bool(Saved,TEXT("compared"));bEscaped=S47J::Bool(Saved,TEXT("escaped"));bVoiced=S47J::Bool(Saved,TEXT("recordedBoyVoice"));
  HoldT=S47J::Num(Saved,TEXT("holdElapsed"));LogBaseline=S47J::Clone(S47J::Obj(Saved,TEXT("commandBaseline")));
  const auto Eye=S47J::Nums(Saved,TEXT("lastFieldEye"));if(Eye.Num()==3)LastFieldEye=FVector(Eye[0],Eye[1],Eye[2]);
  for(const auto& V:S47J::Arr(Saved,TEXT("samples")))if(V&&V->Type==EJson::Object)Samples.Add(S47J::Clone(V->AsObject()));
  for(const auto& V:S47J::Arr(Saved,TEXT("sources")))if(V&&V->Type==EJson::Object){FS47Doc D=DocLoad(V->AsObject());if(!D.Id.IsEmpty())SourcePapers.Add(D);}
  for(const auto& V:S47J::Arr(Saved,TEXT("pointings")))if(V&&V->Type==EJson::Array){const auto Q=V->AsArray();if(Q.Num()==2)RestoredPointings.Add({Q[0]->AsNumber(),Q[1]->AsNumber()});}
  if(M&&M->Web&&RestoredPointings.Num()==M->Web->DishCount())
  {
   for(int32 I=0;I<RestoredPointings.Num();++I)M->Web->SnapDish(I,RestoredPointings[I][0],RestoredPointings[I][1]);
   AppliedRestore=S47J::New();TArray<TSharedPtr<FJsonValue>> Actual;
   for(int32 I=0;I<M->Web->DishCount();++I)Actual.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(M->Web->DishAz(I)),MakeShared<FJsonValueNumber>(M->Web->DishEl(I))}));
   AppliedRestore->SetArrayField(TEXT("pointings"),Actual);
  }
  if((P==EStep::Done||P==EStep::Hold)&&(!bEscaped||!SourcesReady()))P=EStep::Array;
  bResumeHunt=P==EStep::Hunt;
 }
 Refile();KeepSources();
}
void FS47SC08::Stop()
{
 ++Epoch;bActive=false;bDialog=false;Body.Reset();bPlaying=false;
 if(M&&M->SaroNight&&M->SaroNight->ReelTape&&M->SaroNight->ReelTape->bBusy&&Dialogue==TEXT("earl"))M->SaroNight->ReelTape->Stop();
 Dialogue.Empty();Pale.Apply(false,FVector::ZeroVector,0,0,0,0,0,FS47PaleBody::EPose::Walk);bShown=false;Path.Reset();bHavePlayer=false;bResumeHunt=false;
 if(M&&M->SaroNight&&M->SaroNight->Dishes)for(int32 I:Dishes)if(M->SaroNight->Dishes->Dishes.IsValidIndex(I)){M->SaroNight->Dishes->Dishes[I].bRogue=false;M->SaroNight->Dishes->Dishes[I].bBack=false;}
 if(RefLamp)RefLamp.SetVisible(false);if(Isolator)Isolator.SetVisible(false);if(RefLight.IsValid())RefLight->SetIntensity(0);
}
void FS47SC08::KeepSources()
{
 if(!M||!M->Story)return;for(const FS47Doc& D:M->Story->Docs())if(!D.Id.IsEmpty()&&!SourcePapers.ContainsByPredicate([&](const FS47Doc& X){return X.Id==D.Id;}))SourcePapers.Add(D);
}
void FS47SC08::Refile(){if(M&&M->Story)for(const FS47Doc& D:SourcePapers)if(!M->Story->Docs().ContainsByPredicate([&](const FS47Doc& X){return X.Id==D.Id;}))M->Story->AddDoc(D);}
void FS47SC08::File(const FS47Doc& D){if(D.Id.IsEmpty())return;M->Story->AddDoc(D);KeepSources();M->RequestAutosave(false);}
S47J::FObj FS47SC08::Save() const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("step"),int32(P));S47J::Set(O,TEXT("plays"),Plays);S47J::Set(O,TEXT("captures"),Captures);S47J::Set(O,TEXT("first"),First);S47J::Set(O,TEXT("escapedAt"),EscapedAt);
 S47J::Set(O,TEXT("referenceRecorded"),bRefRecorded);S47J::Set(O,TEXT("isolated"),bIsolated);S47J::Set(O,TEXT("labelRead"),bLabelRead);S47J::Set(O,TEXT("compared"),bCompared);S47J::Set(O,TEXT("escaped"),bEscaped);S47J::Set(O,TEXT("recordedBoyVoice"),bVoiced);S47J::Set(O,TEXT("reel_cues"),LastTape);
 S47J::Set(O,TEXT("holdElapsed"),HoldT);S47J::Set(O,TEXT("commandBaseline"),LogBaseline);S47J::Set(O,TEXT("lastFieldEye"),TArray<double>{LastFieldEye.X,LastFieldEye.Y,LastFieldEye.Z});
 TArray<TSharedPtr<FJsonValue>> Rows,Docs,Angles;for(const auto& S:Samples)Rows.Add(MakeShared<FJsonValueObject>(S47J::Clone(S)));for(const FS47Doc& D:SourcePapers)Docs.Add(MakeShared<FJsonValueObject>(DocSave(D)));
 if(M&&M->Web)for(int32 I=0;I<M->Web->DishCount();++I)Angles.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(M->Web->DishAz(I)),MakeShared<FJsonValueNumber>(M->Web->DishEl(I))}));
 O->SetArrayField(TEXT("samples"),Rows);O->SetArrayField(TEXT("sources"),Docs);O->SetArrayField(TEXT("pointings"),Angles);return O;
}
bool FS47SC08::SourcesReady() const
{
 const auto Has=[this](const TCHAR* Id){return SourcePapers.ContainsByPredicate([&](const FS47Doc& D){return D.Id==Id&&!D.Page.IsEmpty();});};
 return Plays>=2&&bLabelRead&&bIsolated&&bRefRecorded&&bCompared&&Samples.Num()==3&&Has(TEXT("witness-reel"))&&Has(TEXT("EV_DALE_ARRAY_SERVICE"))&&Has(TEXT("sc08_comparison"));
}
void FS47SC08::Build()
{
 if(bBuilt||!bActive||!M||!M->Web||!M->Inter||!M->Office||!M->Office->Ready())return;
 AS47Web* W=M->Web;const TCHAR* Names[]={TEXT("office:arrayBoard:A / ROW 1 WEST"),TEXT("office:arrayBoard:B / ROW 1 EAST"),TEXT("office:arrayBoard:REF / R1")};
 for(int32 I=0;I<W->PartCount();++I)if(const auto* N=W->Node(I))for(int32 K=0;K<3;++K)if(N->Name==Names[K])Boards[K]=W->ObjAt(I);
 const auto* RefNode=Boards[2]?W->Node(Boards[2].I):nullptr;
 USceneComponent* RefParent=RefNode?RefNode->Comp.Get():nullptr; // AdoptNode may reallocate the node array.
 auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 Dishes.Reset();for(const TCHAR* Id:{TEXT("S-02"),TEXT("S-03"),TEXT("S-04")})for(int32 I=0;I<W->DishCount();++I)if(W->DishId(I)==Id)Dishes.Add(I);
 if(!Boards[0]||!Boards[1]||!RefParent||!Cube||!Base||Dishes.Num()!=3)
 {if(!bUnavailable){UE_LOG(LogS47SC08,Error,TEXT("S47: SC08 service boards/named near dishes unavailable; originals retained, no completion"));}bUnavailable=true;return;}
 auto Make=[&](const TCHAR* Name,const FVector& At,const FVector& Size,uint32 Color)
 {
  auto* C=NewObject<UStaticMeshComponent>(W,Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(RefParent);C->SetStaticMesh(Cube);
  auto* Mat=UMaterialInstanceDynamic::Create(Base,W);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Color));Mat->SetScalarParameterValue(TEXT("Roughness"),.65);Mat->SetScalarParameterValue(TEXT("EmissivePower"),FString(Name)==TEXT("SC08ReferenceLamp")?.3:0);
  C->SetMaterial(0,Mat);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldScale3D(S47Three::DirToUE(Size));
  return W->ObjAt(W->AdoptNode(C,Boards[2].I,Name));
 };
 RefAt=FVector(-1,-.14,-75.0);RefLamp=Make(TEXT("SC08ReferenceLamp"),RefAt,FVector(.10,.12,.10),0xd8aa56);RefTransform=W->Node(RefLamp.I)->Comp->GetComponentTransform();
 Isolator=Make(TEXT("SC08MonitorInhibit"),FVector(-.76,.25,-75.28),FVector(.05,.12,.045),0x9ba39a);
 auto* Light=NewObject<UPointLightComponent>(W,TEXT("SC08FixedReferenceLight"));Light->SetMobility(EComponentMobility::Movable);Light->SetupAttachment(W->Node(RefLamp.I)->Comp.Get());Light->SetRelativeLocation(FVector(0,0,8));Light->SetIntensityUnits(ELightUnits::Lumens);Light->SetIntensity(35);Light->SetLightColor(FLinearColor(1,.68,.27));Light->SetAttenuationRadius(350);Light->SetCastShadows(true);Light->RegisterComponent();RefLight=Light;
 for(int32 K=0;K<3;++K)
 {
  const FString Id=K==0?TEXT("sc08:a"):K==1?TEXT("sc08:b"):TEXT("sc08:ref");FS47Interactable It;It.Id=Id;It.Object=Boards[K];It.Range=2.6;
  It.Label=[this,Id]{return Label(Id);};It.Use=[this,Id]{Use(Id);};M->Inter->Add(It);
 }
 if(auto* Earl=M->Inter->Get(TEXT("town:amDoor")))
 {OldEarlLabel=Earl->Label;Earl->Label=[this]{const FString Own=Label(TEXT("town:amDoor"));return !Own.IsEmpty()?Own:OldEarlLabel?OldEarlLabel():FString();};}
 for(const TCHAR* Id:{TEXT("truck"),TEXT("dinerTruck")})if(auto* Truck=M->Inter->Get(Id))
 {
  const FString Key(Id);OldTruckLabels.Add(Key,Truck->Label);
  Truck->Label=[this,Key]{const FString Own=Label(Key);const auto* Old=OldTruckLabels.Find(Key);return !Own.IsEmpty()?Own:Old&&*Old?(*Old)():FString();};
 }
 bBuilt=true;bUnavailable=false;
 UE_LOG(LogS47SC08,Display,TEXT("S47: SC08 original Row One boards and S-02/S-03/S-04 bound; fixed reference/inhibit added"));
}
FString FS47SC08::Label(const FString& Id) const
{
 if(!bActive||!M||!M->World||!M->Story||P==EStep::Done)return FString();
 if(!M->World->bDriving && Id==TEXT("truck") && M->World->Area==TEXT("saro") && P==EStep::Earl)return TEXT("Drive to 1420 AM / Earl's labelled reel");
 if(!M->World->bDriving && Id==TEXT("dinerTruck") && M->World->Area==TEXT("diner") && P==EStep::Array)return TEXT("Drive back to SARO / fixed-reference comparison");
 if(Id==TEXT("town:amDoor"))return M->World->Area==TEXT("diner")&&P==EStep::Earl?TEXT("Earl / the isolated 1981 reel"):FString();
 if(M->World->Area!=TEXT("saro")||M->World->bDriving)return FString();
 if(Id==TEXT("sc08:ref")&&(P==EStep::Array||P==EStep::Compare))return TEXT("Fixed reference / service comparison");
 if(P==EStep::Compare&&(Id==TEXT("sc08:a")||Id==TEXT("sc08:b")))return TEXT("Read the local reflector encoders");
 return FString();
}
bool FS47SC08::Use(const FString& Id)
{
 if(Label(Id).IsEmpty())return false;
 if(Id==TEXT("truck")){M->Story->AdvanceClockTo(4*3600.,TEXT("SC08 / leave for Earl after sign-off"));FS47TripOpts O;O.PerMetre=0;O.Hint=TEXT("Leave the truck on the diner lot and walk to the 1420 AM public door. Earl has stayed after sign-off.");M->World->StartTrip(TEXT("saro"),TEXT("diner"),O);return true;}
 if(Id==TEXT("dinerTruck")){M->World->DriveBack();return true;}
 if(Id==TEXT("town:amDoor")){M->Story->AdvanceClockTo(4*3600.,TEXT("SC08 / Earl at the public door after sign-off"));Open(TEXT("earl"));return true;}
 if(Id==TEXT("sc08:ref")){Open(TEXT("reference"));return true;}
 Record(Id==TEXT("sc08:a")?TEXT("A"):TEXT("B"));return true;
}
void FS47SC08::Open(const FString& Panel)
{
 Dialogue=Panel;bDialog=true;const int32 Generation=Epoch;Body=SNew(SVerticalBox);
 M->UI->Open(S47Kit::Shell(M->UI,Panel==TEXT("earl")?TEXT("1420 AM / ISOLATED REEL"):TEXT("ROW ONE / FIXED REFERENCE"),Body.ToSharedRef()),[this,Generation]
 {
  if(Generation!=Epoch)return;bDialog=false;Body.Reset();
  if(bPlaying&&M->Away&&M->Away->LostNights)M->Away->LostNights->StopReel();
 });Refresh();
}
void FS47SC08::Refresh()
{
 if(!Body||!bDialog)return;Body->ClearChildren();
 const auto Text=[&](const FString& S){Body->AddSlot().AutoHeight().Padding(0,4)[S47Kit::Text(S)];};
 const auto Button=[&](const TCHAR* Label,const TCHAR* Id){Body->AddSlot().AutoHeight().Padding(0,3)[S47Kit::Button(Label,[this,Key=FString(Id)]{Choice(Key);})];};
 if(Dialogue==TEXT("earl"))
 {
  Text(TEXT("Earl stays at the public door after sign-off. The transmitter is off.\nEARL: \"1420 AM is the station on your car radio. It isn't 1420.405 megahertz at SARO. Different bands. Don't write them as the same source.\""));
  if(bPlaying){Text(TEXT("The isolated reel turns. Original captions and the original caller playback follow the deck; no new question is put onto the old tape."));Text(LastTape);return;}
  if(!bLabelRead)Button(TEXT("Read the pencil label: 9/22/81. OFF AIR."),TEXT("label"));
  else
  {
   Text(TEXT("The label and the off-air date are identified. A live monitor would only tell you what is on the line now."));
   Button(TEXT("Check the live monitor"),TEXT("live"));
   if(Plays==0)Button(TEXT("Select isolated tape. Play the labelled reel."),TEXT("isolated"));
   else if(Plays==1)Button(TEXT("Ask where he was calling from. Replay the same isolated reel."),TEXT("repeat"));
   else Text(TEXT("Both complete plays contain the same original words and the same holes. The old voice did not answer the new question. Notebook copy filed."));
  }
 }
 else
 {
  Text(TEXT("Keep the reference lamp on REF / R1. The automatic schedule remains held. This local switch inhibits commanded drive transmission; it is not a power cut. The local encoders are read separately from the command history."));
  if(!bIsolated)Button(TEXT("Record command history and latch monitor-only isolation"),TEXT("isolate"));
  else if(!bRefRecorded)Button(TEXT("Record this actual fixed reference and unchanged lamp setting"),TEXT("reference"));
  else if(!M->Story->Docs().ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_DALE_ARRAY_SERVICE");}))Text(TEXT("Read the actual Row One service sheet on Ward's shelf in Operations before using these coordinates."));
  else if(First.IsEmpty()){Button(TEXT("Move the operator, keep REF. Start at A / ROW ONE WEST."),TEXT("first-a"));Button(TEXT("Move the operator, keep REF. Start at B / ROW ONE EAST."),TEXT("first-b"));}
  else Text(FString::Printf(TEXT("First selected: %s. Record that station, the other, then return. Readings retained: %d / 3."),*First,Samples.Num()));
  if(!First.IsEmpty())Button(TEXT("Repeat the control comparison from a fresh command baseline"),TEXT("reset"));
 }
}
void FS47SC08::Choice(const FString& Id)
{
 if(!bActive||!bDialog||bPlaying)return;
 if(Dialogue==TEXT("earl"))
 {
  if(Id==TEXT("label")){bLabelRead=true;M->RequestAutosave(false);Refresh();return;}
  if(Id==TEXT("live")){M->UI->Toast(TEXT("Live monitor: the local transmitter is off. This is not a play of the 1981 reel."),4);return;}
  if((Id==TEXT("isolated")||Id==TEXT("repeat"))&&bLabelRead&&M->Away&&M->Away->LostNights){Play();return;}
 }
 else if(P==EStep::Array||P==EStep::Compare)
 {
  if(Id==TEXT("isolate")){LogBaseline=CommandLog(M->Story);bIsolated=true;if(Isolator)Isolator.SetRotZ(.7);M->RequestAutosave(false);Refresh();return;}
  if(Id==TEXT("reference")&&bIsolated&&FixedReference()){bRefRecorded=true;M->RequestAutosave(false);Refresh();return;}
  if((Id==TEXT("first-a")||Id==TEXT("first-b"))&&bIsolated&&bRefRecorded&&Plays>=2&&M->Story->Docs().ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_DALE_ARRAY_SERVICE");}))
  {First=Id==TEXT("first-a")?TEXT("A"):TEXT("B");Samples.Reset();bCompared=false;P=EStep::Compare;Still=0;KeepSources();M->RequestAutosave(false);Refresh();return;}
  if(Id==TEXT("reset")){First.Empty();Samples.Reset();bCompared=false;bIsolated=bRefRecorded=false;P=EStep::Array;Refresh();return;}
 }
}
void FS47SC08::Play()
{
 if(!M->SaroNight||!M->SaroNight->ReelTape||!M->Away||!M->Away->LostNights)return;
 bPlaying=true;const int32 Generation=Epoch;LastTape.Empty();
 // The existing LostNights adapter owns the recorded caller, original holes and immutable paper.
 M->Away->LostNights->PlayReel([this,Generation](const TArray<FS47LostLine>& Lines)
 {
  if(Generation!=Epoch||!bPlaying)return;FString Page;for(const auto& L:Lines)Page+=L.Key+TEXT(": ")+L.Value+TEXT("\n");LastTape=Page;Refresh();
 },[this,Generation](bool Complete)
 {
  if(Generation!=Epoch)return;bPlaying=false;
  if(Complete&&M->SaroNight&&M->SaroNight->ReelTape)
  {
   const auto& Tape=*M->SaroNight->ReelTape;const FString Actual=CueText(Tape);
   const int32 BoyCount=Tape.Cues.FilterByPredicate([](const FS47TapeCue& C){return C.Kind==TEXT("boy");}).Num();
   const FS47Doc* Existing=SourcePapers.FindByPredicate([](const FS47Doc& D){return D.Id==TEXT("sc08_reel_cues");});
   if(BoyCount==3&&(!Existing||Existing->Page==Actual))
   {
    ++Plays;bVoiced=Tape.bVoiced;FS47Doc D;D.Id=TEXT("sc08_reel_cues");D.Title=TEXT("Reel transport / original cue sequence");D.Kind=TEXT("typed");D.Stamp=TEXT("9/22/81 / ISOLATED PLAYBACK");D.Page=Actual;D.Transcript=Actual;File(D);
    KeepSources();if(Plays>=2)P=EStep::Array;
    UE_LOG(LogS47SC08,Display,TEXT("S47: SC08 original reel complete play=%d recordedVoice=%d, identical cue sequence"),Plays,bVoiced);
   }
   else {UE_LOG(LogS47SC08,Error,TEXT("S47: SC08 isolated reel lacks original boy cues or differs from first play; no completion"));}
  }
  Refresh();M->RequestAutosave(false);
 });Refresh();
}
bool FS47SC08::AtPoint(const FString& Id) const
{
 return M&&M->Player&&M->World&&M->World->Area==TEXT("saro")&&!M->World->bDriving&&FVector2D::Distance(Point(Id),FVector2D(M->Player->Pos.X,M->Player->Pos.Z))<1.05;
}
bool FS47SC08::FixedReference() const
{
 if(!bBuilt||!RefLamp||!RefLight.IsValid()||!M||!M->Web)return false;
 const auto* N=M->Web->Node(RefLamp.I);return N&&N->Comp.IsValid()&&N->Comp->GetComponentTransform().Equals(RefTransform,.001)&&RefLight->IsVisible()&&FMath::Abs(RefLight->Intensity-35)<.001;
}
bool FS47SC08::NoCommand() const{return LogBaseline&&S47J::Write(LogBaseline)==S47J::Write(CommandLog(M?M->Story.Get():nullptr));}
bool FS47SC08::Settled() const
{
 if(!M||!M->Web||!M->Player||Dishes.Num()!=3)return false;
 const FVector Eye=S47Three::ToThree(M->Player->EyeUE());
 for(int32 I:Dishes){const FVector2D Want=Bearing(M->Web->DishX(I),M->Web->DishZ(I),M->Web->DishScale(I),Eye);if(FMath::Abs(Wrap(M->Web->DishAz(I)-Want.X))>.2||FMath::Abs(M->Web->DishEl(I)-Want.Y)>.2)return false;}
 return true;
}
void FS47SC08::Turn(double Dt,const FVector& Eye,bool Home)
{
 for(int32 I:Dishes)
 {
  const FVector2D Want=Home?FVector2D(26,32):Bearing(M->Web->DishX(I),M->Web->DishZ(I),M->Web->DishScale(I),Eye);
  const double Az=M->Web->DishAz(I),El=M->Web->DishEl(I),Speed=Home?6.:18.;
  M->Web->SnapDish(I,Norm(Az+FMath::Clamp(Wrap(Want.X-Az),-Speed*Dt,Speed*Dt)),El+FMath::Clamp(Want.Y-El,-9*Dt,9*Dt));
 }
}
void FS47SC08::Record(const FString& Id)
{
 const FString ExpectedPoint=Samples.Num()==1?(First==TEXT("A")?TEXT("B"):TEXT("A")):First;
 if(First.IsEmpty()||Samples.Num()>=3||Id!=ExpectedPoint||!AtPoint(Id)||!FixedReference()||!NoCommand()||!Settled()||Still<.8)
 {M->UI->Toast(TEXT("Use the selected station, keep REF fixed, and wait for the encoders to settle. A changed command history requires a fresh control comparison."),5);return;}
 auto Row=S47J::New();S47J::Set(Row,TEXT("station"),Id);S47J::Set(Row,TEXT("clock"),M->Story->Clock());S47J::Set(Row,TEXT("player_eye"),TArray<double>{LastFieldEye.X,LastFieldEye.Y,LastFieldEye.Z});
 S47J::Set(Row,TEXT("reference"),TArray<double>{RefAt.X,RefAt.Y,RefAt.Z});S47J::Set(Row,TEXT("reference_lumens"),double(RefLight->Intensity));S47J::Set(Row,TEXT("no_command"),NoCommand());
 TArray<TSharedPtr<FJsonValue>> Angles;
 for(int32 I:Dishes)
 {
  auto R=S47J::New();S47J::Set(R,TEXT("id"),M->Web->DishId(I));S47J::Set(R,TEXT("az"),M->Web->DishAz(I));S47J::Set(R,TEXT("el"),M->Web->DishEl(I));
  const FVector2D RefAim=Bearing(M->Web->DishX(I),M->Web->DishZ(I),M->Web->DishScale(I),RefAt);S47J::Set(R,TEXT("fixed_reference_az"),RefAim.X);Angles.Add(MakeShared<FJsonValueObject>(R));
 }
 Row->SetArrayField(TEXT("encoders"),Angles);Samples.Add(Row);KeepSources();M->RequestAutosave(false);
 if(Samples.Num()<3){M->UI->Toast(FString::Printf(TEXT("%s recorded. Move to %s; keep REF where it is."),*Id,*((Samples.Num()==1)?(First==TEXT("A")?FString(TEXT("B")):FString(TEXT("A"))):First)),5);return;}
 bool Different=false;for(int32 I=0;I<3;++I){const auto R0=S47J::Arr(Samples[0],TEXT("encoders"))[I]->AsObject(),R1=S47J::Arr(Samples[1],TEXT("encoders"))[I]->AsObject(),R2=S47J::Arr(Samples[2],TEXT("encoders"))[I]->AsObject();Different|=FMath::Abs(Wrap(S47J::Num(R0,TEXT("az"))-S47J::Num(R1,TEXT("az"))))>5;if(FMath::Abs(Wrap(S47J::Num(R0,TEXT("az"))-S47J::Num(R2,TEXT("az"))))>2){Samples.Pop();M->UI->Toast(TEXT("The return reading is not repeatable yet. Repeat that station."),4);return;}}
 if(!Different){Samples.Reset();First.Empty();M->UI->Toast(TEXT("The selected positions have not produced distinguishable encoder readings. Repeat the control."),4);return;}
 bCompared=true;FS47Doc D;D.Id=TEXT("sc08_comparison");D.Title=TEXT("Row One / operator and reference comparison");D.Kind=TEXT("hand");D.Stamp=M->Story->ClockText(true);
 D.Page=TEXT("ROW ONE / REYES / 14 APR 1986\nFixed REF lamp unchanged. Local command transmit inhibited; automatic schedule remains held. Actual selected station sequence and independent local encoder readings follow. No new console command in the comparison interval.\n\n");
 for(const auto& S:Samples)D.Page+=S47J::Write(S,true)+TEXT("\n\n");D.Page+=TEXT("In this comparison the near reflectors followed the operator's changed position, not the fixed reference. This does not identify a visitor or establish a cause.");D.Transcript=D.Page;File(D);M->UI->Document(D);
}

bool FS47SC08::ClearWalk(const FVector2D& From,const FVector2D& To) const
{
 if(!M||!M->Web||!M->Player)return false;
 const FVector2D D=To-From;
 for(const auto& C:M->Web->Colliders())if(C.bEnabled)
 {
  double T0=0,T1=1;
  const auto Clip=[&](double Pn,double Q)
  {
   if(FMath::Abs(Pn)<1e-12)return Q>=0;
   const double R=Q/Pn;if(Pn<0){if(R>T1)return false;T0=FMath::Max(T0,R);}else{if(R<T0)return false;T1=FMath::Min(T1,R);}return true;
  };
  if(Clip(-D.X,From.X-(C.MinX-.27))&&Clip(D.X,C.MaxX+.27-From.X)&&Clip(-D.Y,From.Y-(C.MinZ-.27))&&Clip(D.Y,C.MaxZ+.27-From.Y))return false;
 }
 const int32 Count=FMath::Max(1,FMath::CeilToInt(D.Size()/.05));
 for(int32 I=0;I<=Count;++I){const FVector2D At=FMath::Lerp(From,To,double(I)/Count);if(!M->Player->Walkable(At.X,At.Y))return false;}
 return true;
}
bool FS47SC08::ClearSight(const FVector& From,const FVector& To) const
{
 if(!M||!M->Web||!M->Web->GetWorld())return false;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC08Sight),true);
 for(const auto& Part:Pale.All)if(Part.IsValid())Q.AddIgnoredComponent(Part.Get());
 for(int32 I=0;I<24;++I)
 {
  FHitResult Hit;if(!M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(From),S47Three::ToUE(To),ECC_Visibility,Q))return true;
  const auto* C=Hit.GetComponent();if(!C)return false;bool Solid=C->IsVisible();
  const auto* Node=M->Web->Node(M->Web->NodeOf(C));if(Node&&Node->Name.StartsWith(TEXT("proxy")))Solid=false;
  if(const UMaterialInterface* Mat=C->GetMaterial(FMath::Max<int32>(0,Hit.ElementIndex)))
  {
   float Opacity=1;Mat->GetScalarParameterValue(TEXT("Opacity"),Opacity);
   if(Mat->GetBlendMode()==BLEND_Additive||(Mat->GetBlendMode()==BLEND_Translucent&&Opacity<.5f))Solid=false;
  }
  if(Solid)return false;Q.AddIgnoredComponent(C);
 }
 return false;
}
double FS47SC08::Ground(const FVector2D& At) const
{
 return M&&M->Web?US47Night::SaroFloorAt(M->Web.Get(),At.X,At.Y):-.65;
}
bool FS47SC08::SeesPlayer() const
{
 if(!bShown||!M||!M->Player||M->World->bDriving||M->World->Area!=TEXT("saro"))return false;
 const FVector Eye=S47Three::ToThree(M->Player->EyeUE()),O(Pos.X,Ground(Pos)+1.72,Pos.Y);
 const FVector2D D(Eye.X-O.X,Eye.Z-O.Z);const double Dist=D.Size();
 return Dist<32&&(Dist<.8||FVector2D::DotProduct(D/Dist,FVector2D(FMath::Cos(BodyYaw),FMath::Sin(BodyYaw)))>.25)&&ClearSight(O,Eye);
}
int32 FS47SC08::Nearest(const FVector2D& At) const
{
 int32 Best=-1;double Min=1e20;
 for(int32 I=0;I<Nodes().Num();++I)
 {
  const double Dist=FVector2D::DistSquared(At,Nodes()[I].At);
  if(Dist<Min&&ClearWalk(At,Nodes()[I].At)){Best=I;Min=Dist;}
 }
 return Best;
}
void FS47SC08::RouteTo(int32 Goal)
{
 Path.Reset();const int32 Start=Nearest(Pos);if(Start<0||!Nodes().IsValidIndex(Goal))return;
 TArray<int32> Prev;Prev.Init(-2,Nodes().Num());TArray<int32> Q={Start};Prev[Start]=-1;
 for(int32 H=0;H<Q.Num();++H)for(const auto& E:Edges())
 {
  const int32 At=Q[H],Next=E.Key==At?E.Value:E.Value==At?E.Key:-1;
  if(Next<0||Prev[Next]!=-2||!ClearWalk(Nodes()[At].At,Nodes()[Next].At))continue;
  Prev[Next]=At;Q.Add(Next);
 }
 if(Prev[Goal]==-2)return;
 for(int32 N=Goal;N>=0;N=Prev[N])Path.Insert(N,0);
 if(!Path.IsEmpty()&&FVector2D::Distance(Pos,Nodes()[Path[0]].At)<.1)Path.RemoveAt(0);
}
bool FS47SC08::WalkBody(double Speed,double Dt)
{
 if(Path.IsEmpty()){Gait=FMath::Max(0.,Gait-3*Dt);return true;}
 const FVector2D To=Nodes()[Path[0]].At,D=To-Pos;const double Dist=D.Size(),Length=FMath::Min(Dist,Speed*Dt);
 const FVector2D Next=Dist>1e-8?Pos+D/Dist*Length:To;
 if(!ClearWalk(Pos,Next)){Path.Reset();Gait=0;return false;}
 BodyYaw=FMath::Atan2(D.Y,D.X);Pos=Next;Gait=FMath::Min(1.,Gait+3*Dt);Stride+=Length*2.1;
 if(Dist<=Length+.0001)Path.RemoveAt(0);return Path.IsEmpty();
}
void FS47SC08::Enter(EBody Next){Behavior=Next;StateT=0;Path.Reset();}
void FS47SC08::StartHunt(bool Retry)
{
 P=EStep::Hunt;T=StateT=Lost=Noise=0;Gait=Stride=0;Patrol=0;bShown=false;bHavePlayer=false;bResumeHunt=false;
 Pale.Apply(false,FVector::ZeroVector,0,0,0,0,0,FS47PaleBody::EPose::Walk);
 Pos=Nodes()[North].At;BodyYaw=PI/2;LastKnown=Point(First);Enter(EBody::Approach);
 if(!Retry){M->UI->Toast(TEXT("Metal clicks between the footings. Something is standing in the trench. Keep the originals. Find a way back through the gate."),6);}
 UE_LOG(LogS47SC08,Display,TEXT("S47: SC08 array search starts%s; acquired comparison and sources retained"),Retry?TEXT(" again"):TEXT(""));M->RequestAutosave(false);
}
void FS47SC08::Capture()
{
 ++Captures;FadeT=0;M->UI->Fade(true);if(M->Audio)M->Audio->Gasp();
 // Production retry, once per actual capture. Tests never call Place to solve a route.
 const FVector2D Start=Point(First);M->Player->Place(Start.X,Start.Y,0);M->Player->Pitch=-.04;
 StartHunt(true);M->UI->Toast(TEXT("You are back at the marked station. The readings and originals are still in your jacket. The clock has not gone back."),5);
}
void FS47SC08::Escape()
{
 // Escape is credited at the actual south side of the real pedestrian gate, after a built encounter.
 if(!Pale.bBuilt||!bShown||!SourcesReady()||!M->Player||M->World->bDriving||M->World->Area!=TEXT("saro")||Pale.All.Num()!=10)return;
 for(const auto& Part:Pale.All)if(!Part.IsValid()||!Part->GetStaticMesh()||!Part->IsVisible())return;
 const FVector At=M->Player->Pos;
 if(At.Z< -18.7||At.Z> -15||FMath::Abs(At.X+1)>.8||LastFieldEye.Z> -20)return;
 bEscaped=true;EscapedAt=M->Story->Clock();P=EStep::Hold;HoldT=0;Path.Reset();
 M->Story->Note(FString::Printf(TEXT("%s. I came through the array gate with the original reel copy and the A/B/return comparison. The reflectors held the last place in the field, then started back to 026."),*M->Story->ClockText(true)));
 M->UI->Toast(TEXT("Past the gate. The near reflectors hold the last place you stood inside the field."),5);
 UE_LOG(LogS47SC08,Display,TEXT("S47: SC08 actual field escape clock=%.3f at %s; captures=%d"),EscapedAt,*At.ToString(),Captures);M->RequestAutosave(false);
}
void FS47SC08::Hunt(double Dt)
{
 if(!Pale.bBuilt&&!Pale.Build(M->Web.Get(),TEXT("#root"),TEXT("SC08Pale")))
 {
  if(!bUnavailable){UE_LOG(LogS47SC08,Error,TEXT("S47: SC08 body missing; hunt remains incomplete, no escape credited"));}bUnavailable=true;return;
 }
 bUnavailable=false;if(bResumeHunt)StartHunt(true);T+=Dt;StateT+=Dt;
 const FVector2D Pl(M->Player->Pos.X,M->Player->Pos.Z);
 const double Speed=bHavePlayer?FVector2D::Distance(Pl,FVector2D(LastPlayer.X,LastPlayer.Z))/FMath::Max(Dt,.001):0;
 LastPlayer=M->Player->Pos;bHavePlayer=true;Noise=FMath::Max(0.,Noise-Dt);
 const bool Seen=SeesPlayer();const bool Heard=Speed>.4&&Speed<12&&Noise<=0&&FVector2D::Distance(Pos,Pl)<(Speed>2.5?16:8);
 if(Seen){LastKnown=Pl;Lost=0;}else Lost+=Dt;
 if(Heard){LastKnown=Pl;Noise=.6;}
 static const int32 Round[]={Ref,S02Junction,S02E,S02NE,S02NW,S02SW,S02SE,S02E,S02Junction,S03Junction,S03W,S03NW,S03NE,S03SE,S03SW,S03W,S03Junction,SpineMid,Ref,North};
 switch(Behavior)
 {
 case EBody::Approach:
  if(T>=2.5){bShown=true;Enter(EBody::Search);RouteTo(Ref);}break;
 case EBody::Search:
  if(StateT<2.2){Gait=0;break;}
  if(Seen){Enter(EBody::Chase);break;}
  if(Heard){RouteTo(Nearest(LastKnown));}
  if(WalkBody(1.15,Dt))
  {
   // A search is readable: stop and turn between routes, rather than teleporting onto the player.
   if(StateT>1.2){RouteTo(Round[Patrol++%UE_ARRAY_COUNT(Round)]);StateT=0;}
  }
  break;
 case EBody::Chase:
 {
  if(M->World->bDriving||Pl.Y> -19){Enter(EBody::Search);RouteTo(S03Junction);break;}
  const double Dist=FVector2D::Distance(Pos,Pl);
  if(Dist<.9&&Seen){Enter(EBody::Grab);break;}
  const FVector2D Delta=LastKnown-Pos;const double Len=Delta.Size(),Move=FMath::Min(Len,2.1*Dt);const FVector2D Next=Len>.0001?Pos+Delta/Len*Move:Pos;
  if(ClearWalk(Pos,Next))
  {BodyYaw=FMath::Atan2(Delta.Y,Delta.X);Pos=Next;Gait=1.2;Stride+=Move*2.1;Path.Reset();}
  else
  {if(Path.IsEmpty())RouteTo(Nearest(LastKnown));WalkBody(2.1,Dt);}
  if(Lost>3.2){Enter(EBody::Search);RouteTo(Nearest(LastKnown));}break;
 }
 case EBody::Grab:
  Gait=0;
  // Each frame revalidates a real reachable player. Running away cancels an unfinished grab.
  if(M->World->bDriving||Pl.Y> -19||FVector2D::Distance(Pos,Pl)>.9||!Seen){Enter(EBody::Chase);break;}
  if(StateT>=.7){Capture();return;}break;
 case EBody::Leave:
  WalkBody(1.15,Dt);break;
 }
 Pale.Apply(bShown,FVector(Pos.X,Ground(Pos),Pos.Y),BodyYaw,BodyYaw,Gait,Stride,T,Behavior==EBody::Grab?FS47PaleBody::EPose::Grab:Behavior==EBody::Chase?FS47PaleBody::EPose::Chase:FS47PaleBody::EPose::Walk);
 Escape();
}
bool FS47SC08::Step(double Dt)
{
 if(!bActive||!M||!M->Story||!M->World||!M->Player||!M->UI)return false;
 Refile();if(P==EStep::Done)return true;
 Build();
 const bool Saro=M->Mode==US47Main::EMode::Play&&M->World->Area==TEXT("saro");
 if(RefLamp)RefLamp.SetVisible(Saro);if(Isolator)Isolator.SetVisible(Saro);if(RefLight.IsValid()){RefLight->SetVisibility(Saro);RefLight->SetIntensity(Saro?35:0);}
 if(FadeT>=0){FadeT+=Dt;if(FadeT>=1.2){FadeT=-1;M->UI->Fade(false);}}
 const bool Held=M->Mode!=US47Main::EMode::Play||M->bPausedByMenu||M->ModalOpen()||M->Story->IsCinematic()||(M->Flow&&M->Flow->bStarting)||M->World->bBusy||FadeT>=0;
 if(Held)return false;
 KeepSources();
 if(!Saro||M->World->bDriving)
 {Pale.Apply(false,FVector::ZeroVector,0,0,0,0,0,FS47PaleBody::EPose::Walk);return false;}
 const FVector Eye=S47Three::ToThree(M->Player->EyeUE());
 if(P==EStep::Compare||P==EStep::Hunt)
 {
  Turn(Dt,Eye);if(M->Player->Pos.Z<= -20)LastFieldEye=Eye;
  const bool Stable=bHavePlayer&&FVector::DistSquared(M->Player->Pos,LastPlayer)<.0001;
  if(P==EStep::Compare){Still=Stable?Still+Dt:0;LastPlayer=M->Player->Pos;bHavePlayer=true;}
  if(M->SaroNight&&M->SaroNight->Dishes)for(int32 I:Dishes)if(M->SaroNight->Dishes->Dishes.IsValidIndex(I))M->SaroNight->Dishes->Dishes[I].bRogue=true;
 }
 if(P==EStep::Compare&&SourcesReady())StartHunt(false);
 if(P==EStep::Hunt)Hunt(Dt);
 else if(P==EStep::Hold)
 {
  HoldT+=Dt;Turn(Dt,LastFieldEye,HoldT>=4);
  if(HoldT>4){Behavior=EBody::Leave;if(Path.IsEmpty()&&FVector2D::Distance(Pos,Nodes()[North].At)>.2)RouteTo(North);WalkBody(1.15,Dt);}
  Pale.Apply(bShown,FVector(Pos.X,Ground(Pos),Pos.Y),BodyYaw,BodyYaw,Gait,Stride,T,FS47PaleBody::EPose::Walk);
  bool Home=Dishes.Num()==3;for(int32 I:Dishes)Home&=FMath::Abs(Wrap(M->Web->DishAz(I)-26))<.05&&FMath::Abs(M->Web->DishEl(I)-32)<.05;
  if(HoldT>4&&Home)
  {
   P=EStep::Done;Stop();M->Story->AdvanceClockTo(4*3600.+20*60,TEXT("SC08 / actual escape and return to 026"));M->RequestAutosave(false);return true;
  }
 }
 return false;
}
FString FS47SC08::Objective() const
{
 switch(P)
 {
 case EStep::Earl:return TEXT("SC08 // DRIVE TO 1420 AM. EARL HAS A LABELLED 1981 REEL AT THE PUBLIC DOOR AFTER SIGN-OFF");
 case EStep::Array:return TEXT("SC08 // KEEP THE REEL COPY. READ THE ROW ONE SERVICE SHEET IN OPERATIONS; A, B AND REF ARE SEPARATE FIELD MARKS");
 case EStep::Compare:return FString::Printf(TEXT("SC08 // REF STAYS. RECORD %s, %s, THEN %s. WAIT FOR ACTUAL LOCAL ENCODERS; SEND NO COMMAND"),*First,First==TEXT("A")?TEXT("B"):TEXT("A"),*First);
 case EStep::Hunt:return TEXT("SC08 // GET BACK THROUGH THE WALK GATE WITH THE ORIGINALS. OPAQUE FOOTINGS GIVE COVER; READ THE SEARCH BEFORE CROSSING");
 case EStep::Hold:return TEXT("SC08 // PAST THE GATE. WATCH THE REFLECTORS HOLD, THEN GO BACK TO 026");
 default:return FString();
 }
}
TArray<FS47Task> FS47SC08::Tasks() const
{
 return {{TEXT("Identify Earl's OFF AIR / 9/22/81 label; live monitor is a different source"),bLabelRead},
  {TEXT("Hear two complete, identical plays of the original isolated reel"),Plays>=2},
  {TEXT("Retain the physical Row One service sheet and record the fixed reference"),bRefRecorded&&SourcePapers.ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_DALE_ARRAY_SERVICE");})},
  {TEXT("Move the operator A/B/return; keep REF and independent command history unchanged"),bCompared},
  {TEXT("Escape the actual field search with the sources; wait for return to 026"),P==EStep::Done}};
}
S47J::FObj FS47SC08::Readback() const
{
 auto O=Save();S47J::Set(O,TEXT("active"),bActive);S47J::Set(O,TEXT("built"),bBuilt);S47J::Set(O,TEXT("unavailable"),bUnavailable);S47J::Set(O,TEXT("body_built"),Pale.bBuilt);S47J::Set(O,TEXT("body_shown"),bShown);S47J::Set(O,TEXT("body_rendered"),Pale.Rendered());
 S47J::Set(O,TEXT("body_state"),int32(Behavior));S47J::Set(O,TEXT("body_at"),TArray<double>{Pos.X,Pos.Y});S47J::Set(O,TEXT("body_elapsed"),StateT);S47J::Set(O,TEXT("fixed_reference"),FixedReference());S47J::Set(O,TEXT("command_history_unchanged"),NoCommand());S47J::Set(O,TEXT("restore_applied_actual"),AppliedRestore);
 TArray<TSharedPtr<FJsonValue>> Native;
 if(M&&M->Web)for(int32 I:Dishes)
 {
  auto Row=S47J::New();S47J::Set(Row,TEXT("id"),M->Web->DishId(I));S47J::Set(Row,TEXT("az"),M->Web->DishAz(I));S47J::Set(Row,TEXT("el"),M->Web->DishEl(I));TArray<TSharedPtr<FJsonValue>> Parts;
  for(int32 PartIndex:{2,3})
  {
   const auto* N=M->Web->Node(M->Web->Handle(FString::Printf(TEXT("ext.dishArray.parts[%d].mesh"),PartIndex)));
   const auto* C=N?Cast<UInstancedStaticMeshComponent>(N->Comp.Get()):nullptr;FTransform X;
   if(C&&C->GetInstanceTransform(I,X,true)){auto Part=S47J::New();S47J::Set(Part,TEXT("component"),C->GetPathName());S47J::Set(Part,TEXT("mesh"),C->GetStaticMesh()?C->GetStaticMesh()->GetPathName():FString());S47J::Set(Part,TEXT("instance"),I);S47J::Set(Part,TEXT("world_transform"),X.ToString());Parts.Add(MakeShared<FJsonValueObject>(Part));}
  }
  Row->SetArrayField(TEXT("native_parts"),Parts);Native.Add(MakeShared<FJsonValueObject>(Row));
 }
 O->SetArrayField(TEXT("actual_native_dishes"),Native);return O;
}
bool FS47SC08::SelfTest(TArray<FString>& Lines)
{
 const auto E=FVector(-8,.97,-74),F=FVector(18,.97,-74);const FVector2D One=Bearing(-16,-52,.85,E),Two=Bearing(-16,-52,.85,F);
 bool Good=FMath::Abs(Wrap(One.X-Two.X))>25&&One.Y>=S47Follow::EL_MIN&&One.Y<=88&&Point(TEXT("REF"))==FVector2D(-1,-74)&&Nodes().Num()==18&&Edges().Num()==19;
 auto Cohort=S47J::New(),Case=S47J::New(),C2=S47J::New(),Spine=S47J::New(),Office=S47J::New();
 S47J::Set(Cohort,TEXT("case"),Case);S47J::Set(Case,TEXT("ch2"),C2);S47J::Set(Cohort,TEXT("spine"),Spine);S47J::Set(Cohort,TEXT("office"),Office);
 S47J::Set(Spine,TEXT("v"),1);S47J::Set(Spine,TEXT("sc04"),2);S47J::Set(Spine,TEXT("sc08"),1);S47J::Set(Office,TEXT("departed"),true);
 S47J::Set(C2,TEXT("sc07Version"),1);S47J::Set(C2,TEXT("stage"),TEXT("complete"));for(const TCHAR* K:{TEXT("binderCompared"),TEXT("p04"),TEXT("p05"),TEXT("called")})S47J::Set(C2,K,true);
 TArray<TSharedPtr<FJsonValue>> Papers;for(const TCHAR* Id:{TEXT("obs_runbinder"),TEXT("EV_DALE_INDEX"),TEXT("EV_DALE_NOTE")}){auto D=S47J::New();S47J::Set(D,TEXT("id"),Id);S47J::Set(D,TEXT("page"),TEXT("EXPLICIT JSON SELFTEST FIXTURE; never filed into a game"));Papers.Add(MakeShared<FJsonValueObject>(D));}C2->SetArrayField(TEXT("sources"),Papers);
 Good&=CanAdoptSC07(Cohort)&&!CanAdoptSC07(S47J::New());S47J::Set(Office,TEXT("departed"),false);Good&=!CanAdoptSC07(Cohort);S47J::Set(Office,TEXT("departed"),true);S47J::Set(C2,TEXT("called"),false);Good&=!CanAdoptSC07(Cohort);
 Lines.Add(FString::Printf(TEXT("%s SC08: actual operator-bearing maths, fixed REF, narrow SC07 custody adoption and authored 18-node field routes; native geometry/LOS unverified"),Good?TEXT("PASS"):TEXT("FAIL")));return Good;
}

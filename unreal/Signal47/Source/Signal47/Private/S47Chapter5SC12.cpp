// SC12, Tom 29 October 2026: a safe diner, separate sources, a receipted warning and a chosen road.
#include "S47Chapter5.h"
#include "S47Canvas.h"
#include "S47Diner.h"
#include "S47Flow.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Spine.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
 constexpr double DeputyArrival = 5 * 3600 + 15 * 60;
 const TCHAR* SourceDocs[] = { TEXT("HR_SARO_ORIGINAL"), TEXT("HR_HONDO_REMOTE"), TEXT("HR_SITE11_REMOTE") };
 const TCHAR* SourceIds[] = { TEXT("SARO07-860414-RX01"), TEXT("HONDO-860414-LOG01"), TEXT("SITE11-860414-TR01") };
 const TCHAR* FirstBoundary = TEXT("04:56:08 MST, 14 APR 1986");
 const FS47Doc* Source(const AS47Story* H, int32 I)
 {
  return H && I >= 0 && I < 3 ? H->Docs().FindByPredicate([I](const FS47Doc& D) { return D.Id == SourceDocs[I]; }) : nullptr;
 }
 FString TextOf(const FS47Doc* D) { return D ? D->Page + TEXT("\n") + D->Transcript : FString(); }
 FS47Doc Paper(const TCHAR* Id, const TCHAR* Title, const FString& Stamp, const FString& Page)
 {
  FS47Doc D; D.Id = Id; D.Title = Title; D.Kind = TEXT("typed"); D.Stamp = Stamp; D.Page = Page; D.Transcript = Page; return D;
 }
 S47J::FObj SavePaper(const FS47Doc& D)
 {
  auto O = S47J::New();
  S47J::Set(O,TEXT("id"),D.Id); S47J::Set(O,TEXT("title"),D.Title); S47J::Set(O,TEXT("kind"),D.Kind); S47J::Set(O,TEXT("stamp"),D.Stamp);
  S47J::Set(O,TEXT("page"),D.Page); S47J::Set(O,TEXT("transcript"),D.Transcript); S47J::Set(O,TEXT("image"),D.Image); S47J::Set(O,TEXT("paper"),D.Paper); S47J::Set(O,TEXT("ratio"),D.PaperRatio);
  TArray<TSharedPtr<FJsonValue>> Sheets;
  for (const auto& S : D.Sheets) { auto P = S47J::New(); S47J::Set(P,TEXT("page"),S.Page); S47J::Set(P,TEXT("paper"),S.Paper); S47J::Set(P,TEXT("strip"),S.Strip); S47J::Set(P,TEXT("term"),S.Term); Sheets.Add(MakeShared<FJsonValueObject>(P)); }
  O->SetArrayField(TEXT("sheets"),Sheets); return O;
 }
 FS47Doc LoadPaper(const S47J::FObj& O)
 {
  FS47Doc D; D.Id=S47J::Str(O,TEXT("id")); D.Title=S47J::Str(O,TEXT("title")); D.Kind=S47J::Str(O,TEXT("kind")); D.Stamp=S47J::Str(O,TEXT("stamp"));
  D.Page=S47J::Str(O,TEXT("page")); D.Transcript=S47J::Str(O,TEXT("transcript")); D.Image=S47J::Str(O,TEXT("image")); D.Paper=S47J::Str(O,TEXT("paper")); D.PaperRatio=S47J::Num(O,TEXT("ratio"));
  for (const auto& V : S47J::Arr(O,TEXT("sheets"))) if (V && V->Type == EJson::Object) { const auto P=V->AsObject(); D.Sheets.Add({S47J::Str(P,TEXT("page")),S47J::Str(P,TEXT("paper")),S47J::Str(P,TEXT("strip")),S47J::Str(P,TEXT("term"))}); }
  return D;
 }
 S47Panels::FTalkLine Line(const TCHAR* Who, const FString& Text) { return { FString(Who), Text }; }
}

S47J::FObj FS47SC12State::Save() const
{
 if (Version != 1) return nullptr;
 auto O = Extra ? S47J::Clone(Extra) : S47J::New();
 S47J::Set(O, TEXT("v"), 1); S47J::Set(O, TEXT("laid"), bLaid);
 S47J::Set(O, TEXT("read"), ReadMask); S47J::Set(O, TEXT("checks"), CheckMask); S47J::Set(O, TEXT("compared"), bCompared);
 S47J::Set(O, TEXT("warningSent"), bWarningSent); S47J::Set(O, TEXT("warningStamp"), WarningStamp);
 S47J::Set(O, TEXT("deputyHere"), bDeputyHere); S47J::Set(O, TEXT("patrolRead"), bPatrolRead); S47J::Set(O, TEXT("patrolStamp"), PatrolStamp);
 S47J::Set(O, TEXT("courier104"), bCourier); S47J::Set(O, TEXT("routeReadback"), bRouteReadback); S47J::Set(O, TEXT("routeStamp"), RouteStamp);
 TArray<TSharedPtr<FJsonValue>> Papers; for (const auto& D : SourcePapers) Papers.Add(MakeShared<FJsonValueObject>(SavePaper(D))); O->SetArrayField(TEXT("sources"),Papers);
 return O;
}

FS47SC12State FS47SC12State::Load(const S47J::FObj& O)
{
 FS47SC12State F;
 if (S47J::Num(O, TEXT("v")) != 1) return F;
 F.Version = 1; F.Extra = S47J::Clone(O);
 F.bLaid = S47J::Bool(O, TEXT("laid"));
 const auto Mask = [&O](const TCHAR* K) { const double N = S47J::Num(O, K, -1); return FMath::IsFinite(N) && N >= 0 && N <= 7 && N == FMath::FloorToDouble(N) ? int32(N) : 0; };
 F.ReadMask = Mask(TEXT("read")); F.CheckMask = Mask(TEXT("checks"));
 F.bCompared = S47J::Bool(O, TEXT("compared")) && F.bLaid && F.ReadMask == 7 && F.CheckMask == 7;
 F.WarningStamp = S47J::Str(O, TEXT("warningStamp")); F.PatrolStamp = S47J::Str(O, TEXT("patrolStamp")); F.RouteStamp = S47J::Str(O, TEXT("routeStamp"));
 F.bWarningSent = S47J::Bool(O, TEXT("warningSent")) && F.bCompared && !F.WarningStamp.IsEmpty();
 F.bDeputyHere = S47J::Bool(O, TEXT("deputyHere"));
 F.bPatrolRead = S47J::Bool(O, TEXT("patrolRead")) && F.bDeputyHere && !F.PatrolStamp.IsEmpty();
 F.bCourier = S47J::Bool(O, TEXT("courier104")) && F.bWarningSent && F.bPatrolRead;
 F.bRouteReadback = S47J::Bool(O, TEXT("routeReadback")) && F.bCourier && !F.RouteStamp.IsEmpty();
 for (const auto& V : S47J::Arr(O,TEXT("sources"))) if (V && V->Type == EJson::Object)
 {
  const FS47Doc D=LoadPaper(V->AsObject());
  for (int32 I=0;I<3;++I) if (D.Id==SourceDocs[I] && TextOf(&D).Contains(FString(TEXT("SOURCE ID / "))+SourceIds[I],ESearchCase::CaseSensitive)
   && !F.SourcePapers.ContainsByPredicate([&](const FS47Doc& P){return P.Id==D.Id;})) F.SourcePapers.Add(D);
 }
 return F;
}

void US47Chapter5::CaptureSC12Sources()
{
 if (!RevisedDiner()) return;
 for (int32 I=0;I<3;++I)
 {
  const FS47Doc* D=Source(Host,I);
  if (D && TextOf(D).Contains(FString(TEXT("SOURCE ID / "))+SourceIds[I],ESearchCase::CaseSensitive)
   && !S.SC12.SourcePapers.ContainsByPredicate([&](const FS47Doc& P){return P.Id==D->Id;})) S.SC12.SourcePapers.Add(*D);
 }
}

bool US47Chapter5::HasSC12Sources() const
{
 for (int32 I = 0; I < 3; ++I)
 {
  const FS47Doc* D = Source(Host, I);
  if (!D || !TextOf(D).Contains(FString(TEXT("SOURCE ID / ")) + SourceIds[I], ESearchCase::CaseSensitive)) return false;
 }
 return true;
}

bool US47Chapter5::SourcesVerified() const
{
 return RevisedDiner() && HasSC12Sources() && S.SC12.bCompared && S.SC12.ReadMask == 7 && S.SC12.CheckMask == 7;
}

bool US47Chapter5::ReadyForRoad() const
{
 return SourcesVerified() && S.SC12.bWarningSent && S.SC12.bPatrolRead && S.bP13 && S.SC12.bCourier && S.SC12.bRouteReadback;
}

void US47Chapter5::UpdateRoadReady()
{
 if (RevisedDiner() && S.Stage != TEXT("complete")) S.Stage = ReadyForRoad() ? TEXT("road") : S.bArrived ? TEXT("diner") : TEXT("to-truck");
}

FS47Obj US47Chapter5::SC12Object(const FString& Id) const
{
 const FS47Obj* O = SC12Props.Find(Id); return O ? *O : FS47Obj();
}

void US47Chapter5::BuildSC12Props()
{
 if (!RevisedDiner() || SC12Props.Num() || !M || !M->Web || !M->Inter || !Diner()) return;
 AS47Web* W = M->Web; const FS47Obj Group = Diner()->Group;
 const auto* N = W->Node(Group.I); USceneComponent* Parent = N ? N->Comp.Get() : nullptr;
 UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 if (!Parent || !Cube || !Cylinder || !Sphere || !Base) return;
 TWeakObjectPtr<US47Chapter5> Weak = this;
 const auto Mesh = [&](const FString& Id, UStaticMesh* Asset, const FVector& At, const FVector& Size, uint32 Color, bool Usable)
 {
  auto* C = NewObject<UStaticMeshComponent>(W, *FString::Printf(TEXT("SC12_%s"), *Id));
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Parent); C->SetStaticMesh(Asset);
  auto* Mat = UMaterialInstanceDynamic::Create(Base, W); Mat->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Color));
  Mat->SetScalarParameterValue(TEXT("Roughness"), .88f); C->SetMaterial(0, Mat);
  C->SetCollisionEnabled(Usable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
  C->SetCollisionResponseToAllChannels(ECR_Ignore); if (Usable) C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
  C->RegisterComponent(); C->SetWorldLocation(S47Three::ToUE(Group.LocalToWorld(At))); C->SetWorldScale3D(S47Three::DirToUE(Size));
  FS47Obj O = W->ObjAt(W->AdoptNode(C, Group.I, FString(TEXT("sc12:")) + Id)); SC12Props.Add(Id, O); O.SetVisible(false); return O;
 };
 const auto Labelled = [&](const FString& Id, const FS47Obj& O, TFunction<FString()> Label, TFunction<void()> Use)
 {
  FS47Interactable I; I.Id = FString(TEXT("sc12:")) + Id; I.Object = O; I.Range = 2.6;
  I.Label = [Weak, Label] { return Weak.IsValid() && Weak->bActive && Weak->RevisedDiner() && Weak->InDiner() ? Label() : FString(); };
  I.Use = MoveTemp(Use); M->Inter->Add(I);
 };
 for (int32 I = 0; I < 3; ++I)
 {
  const FString Id = FString::Printf(TEXT("source%d"), I);
  const FS47Obj O = Mesh(Id, Cube, FVector(.34 + I * .35, .8055, 3.05), FVector(.285, .006, .35), 0xeee6d0, true);
  Labelled(Id, O, [Weak, I] { return Weak.IsValid() && Weak->S.SC12.bLaid && Source(Weak->Host, I) ? FString::Printf(TEXT("Read %s"), SourceIds[I]) : FString(); },
   [Weak, I] { if (Weak.IsValid()) Weak->ReadSource(I); });
  // Authored paper, not a photograph or a new source: the carried originals remain in Story::Docs.
  FS47Canvas Page(512, 640); Page.SetFill(0xeee6d0); Page.FillRect(0, 0, 512, 640); Page.SetFill(0x243128);
  Page.SetFont(TEXT("SpecialElite-Regular.ttf"), 23); Page.FillText(TEXT("14 APR 1986"), 28, 56);
  Page.FillText(SourceIds[I], 28, 105, 456); Page.FillText(I == 0 ? TEXT("RECEIVER + ROOM NOTE") : I == 1 ? TEXT("LOCAL EVENT LOGGER") : TEXT("LOCAL TRANSIT RECORD"), 28, 155, 456);
  Page.SetStroke(0x49564d); Page.LineWidth = 2;
  for (int32 Row = 0; Row < 10; ++Row) { Page.BeginPath(); Page.MoveTo(30, 210 + Row * 31); Page.LineTo(470 - (Row % 3) * 38, 210 + Row * 31); Page.Stroke(); }
  if (UTexture2D* T = S47Image::FromPixels(Page.Px, Page.W, Page.H)) O.SetMap(T);
 }
 const FS47Obj Patrol = Mesh(TEXT("patrol"), Cube, FVector(-4.5, 1.004, 1.25), FVector(.31, .008, .39), 0xf0e4c9, true);
 Labelled(TEXT("patrol"), Patrol, [Weak] { return Weak.IsValid() && Weak->S.SC12.bDeputyHere ? FString(TEXT("Read the dated patrol sheet")) : FString(); }, [Weak] { if (Weak.IsValid()) Weak->ReadPatrol(); });
 FS47Canvas Page(512, 640); Page.SetFill(0xf0e4c9); Page.FillRect(0, 0, 512, 640); Page.SetFill(0x1c3028); Page.SetFont(TEXT("SpecialElite-Regular.ttf"), 27);
 Page.FillText(TEXT("COUNTY SHERIFF"), 25, 58); Page.FillText(TEXT("14 APR 1986"), 25, 118); Page.FillText(TEXT("04:15 MST / PATROL"), 25, 188);
 Page.FillText(TEXT("NORTH / OLD ROSWELL"), 25, 260, 462); Page.FillText(TEXT("NO OBSTRUCTION THEN"), 25, 330, 462); Page.FillText(TEXT("ONE DRIVE. NOT A"), 25, 432); Page.FillText(TEXT("SAFETY GUARANTEE."), 25, 482);
 if (UTexture2D* T = S47Image::FromPixels(Page.Px, Page.W, Page.H)) Patrol.SetMap(T);
 // The same modest uniform/hat motif as SC03; its earlier components and scene stay untouched.
 Mesh(TEXT("deputyBody"), Cylinder, FVector(-3.45, .92, 2.0), FVector(.40, .92, .32), 0x8a7a52, true);
 Mesh(TEXT("deputyHead"), Sphere, FVector(-3.45, 1.66, 2.0), FVector(.21, .25, .23), 0xb99478, false);
 Mesh(TEXT("deputyHat"), Cylinder, FVector(-3.45, 1.80, 2.0), FVector(.42, .05, .42), 0x5a4a32, false);
 Labelled(TEXT("deputy"), SC12Object(TEXT("deputyBody")), [Weak] { return Weak.IsValid() && Weak->S.SC12.bDeputyHere ? FString(TEXT("The deputy / patrol sheet")) : FString(); }, [Weak] { if (Weak.IsValid()) Weak->ReadPatrol(); });
}

void US47Chapter5::ApplySC12Props()
{
 for (const auto& Entry : SC12Props)
 {
  const bool SourcePaper = Entry.Key.StartsWith(TEXT("source"));
  const int32 I = SourcePaper ? FCString::Atoi(*Entry.Key.Mid(6)) : -1;
  const bool Show = bActive && RevisedDiner() && S.bArrived && (SourcePaper ? S.SC12.bLaid && Source(Host, I) != nullptr : S.SC12.bDeputyHere);
  FS47Obj O = Entry.Value; O.SetVisible(Show);
  if (const auto* N = M && M->Web ? M->Web->Node(O.I) : nullptr) if (auto* C = Cast<UStaticMeshComponent>(N->Comp.Get()))
   C->SetCollisionEnabled(Show && (SourcePaper || Entry.Key == TEXT("patrol") || Entry.Key == TEXT("deputyBody")) ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
 }
}

void US47Chapter5::StepSC12(double Dt)
{
 (void)Dt;
 // Flow enables the spine just after a direct developer jump, before this first playable update.
 if (bFreshChapter && S.SC12.Version == 0 && Host && Host->bEveningTimeline && M && M->Spine && M->Spine->UsesRevisedSC12())
 {
  S.SC12.Version = 1; MapUrl.Reset(); Save();
 }
 if (!RevisedDiner()) return;
 // The player's own car is allowed too. Entering the real diner on foot is an arrival, not only
 // finishing the service truck's scripted trip.
 if (!S.bArrived && InDiner() && !Driving() && M->Player && M->World && M->World->Indoors(M->Player->Pos)) ArrivedDiner();
 BuildSC12Props();
 if (!S.SC12.bDeputyHere && Host && Host->Clock() >= DeputyArrival)
 {
  S.SC12.bDeputyHere = true;
  Note(FString::Printf(TEXT("%s. The deputy comes into Mesa Diner and puts his dated patrol sheet on the counter. He drove the north and old Roswell roads at 04:15, an hour before his arrival. One report, not a promise."), *Time()));
  if (InDiner()) Toast(TEXT("The deputy comes in. A dated patrol sheet lands on the counter."), 4.5f);
  Save();
 }
 UpdateRoadReady(); ApplySC12Props();
}

bool US47Chapter5::SC12Choice(const FString& Id)
{
 if (!bActive || !RevisedDiner() || !View().Choices.ContainsByPredicate([&](const S47Panels::FTalkChoice& C) { return C.Id == Id; })) return false;
 Choose(Id); if (Panel && Panel->Refresh) Panel->Refresh(); return true;
}

void US47Chapter5::OpenSources()
{
 Who = TEXT("sources");
 Lines = { Line(TEXT(""), TEXT("The folder stays on the booth table. SARO, Hondo and Site 11 have separate source records. Check who made each one, their clocks and whether one feed could explain them all.")) };
 if (!S.SC12.bLaid)
 {
  S.SC12.bLaid = true; ApplySC12Props(); Save();
 }
 if (!HasSC12Sources()) Lines.Add(Line(TEXT(""), TEXT("A complete independent source is missing. A telex saying somebody called is not the missing original. Keep the local warning separate; do not invent a network conclusion.")));
 // Reuse the established talk panel and its normal choices, without any voice transaction.
 if (!M || !M->UI) return;
 TWeakObjectPtr<US47Chapter5> W = this; S47Panels::FTalk O; O.Title = TEXT("MESA DINER / THREE SEPARATE SOURCES");
 O.View = [W] { return W.IsValid() ? W->View() : S47Panels::FTalkView(); };
 O.OnChoice = [W](const FString& Id) { if (W.IsValid()) W->Choose(Id); };
 O.OnClose = [W] { if (W.IsValid()) { W->Panel.Reset(); W->Who.Reset(); W->PendingReadback = 0; } };
 Panel = S47Panels::Talk(M->UI, O);
}

void US47Chapter5::ReadSource(int32 Index)
{
 if (!RevisedDiner() || !bActive || !S.SC12.bLaid || !InDiner()) return;
 const FS47Doc* D = Source(Host, Index);
 if (!D) { Toast(TEXT("That source is not in the folder."), 3); return; }
 // Store the actual carried paper; merely finding a label never verifies the comparison.
 S.SC12.ReadMask |= (1 << Index); Save();
 if (M && M->UI) M->UI->Document(*D);
}

void US47Chapter5::ReadPatrol()
{
 if (!RevisedDiner() || !bActive || !InDiner() || !S.SC12.bDeputyHere || !Host || Host->Clock() < DeputyArrival) return;
 if (!S.SC12.bPatrolRead)
 {
  S.SC12.bPatrolRead = true; S.SC12.PatrolStamp = Time(); File(PatrolDoc());
  Note(FString::Printf(TEXT("%s. The deputy's patrol sheet: 14 APR 1986, 04:15 MST, north road and old Roswell road. No obstruction then. He arrived at 05:15. One drive is not a guarantee about the road now."), *Time())); Save();
 }
 if (M && M->UI) M->UI->Document(PatrolDoc());
}

S47Panels::FTalkView US47Chapter5::SC12View() const
{
 S47Panels::FTalkView V; V.Status = Objective(); V.Lines = Lines;
 if (Who == TEXT("sources"))
 {
  for (int32 I = 0; I < 3; ++I) if (Source(Host, I)) V.Choices.Add({ FString::Printf(TEXT("read%d"), I), FString::Printf(TEXT("Read %s"), SourceIds[I]) });
  if (S.SC12.ReadMask == 7 && HasSC12Sources())
  {
   if (!(S.SC12.CheckMask & 1)) V.Choices.Add({ TEXT("origins"), TEXT("Compare the three local origins") });
   if (!(S.SC12.CheckMask & 2)) V.Choices.Add({ TEXT("clocks"), TEXT("Compare the clock checks and boundaries") });
   if (!(S.SC12.CheckMask & 4)) V.Choices.Add({ TEXT("feeds"), TEXT("Test the shared-feed explanation") });
   if (S.SC12.CheckMask == 7 && !S.SC12.bCompared) V.Choices.Add({ TEXT("compare"), TEXT("Record the supported comparison, with its limits") });
  }
  if (S.SC12.bWarningSent && S.SC12.bPatrolRead) V.Choices.Add({ TEXT("map"), TEXT("Align E09A on the E14 route map (P13)") });
  V.Choices.Add({ TEXT("bye"), TEXT("Keep the folder together") }); return V;
 }
 if (!S.SC12.bWarningSent)
 {
  if (PendingReadback == 1) V.Choices.Add({ TEXT("ack-warning"), TEXT("Confirm all three IDs and 04:56:08 in Ward's readback") });
  else if (SourcesVerified()) V.Choices.Add({ TEXT("send-warning"), TEXT("Read the three source IDs and the first boundary time") });
  else V.Choices.Add({ TEXT("local-warning"), TEXT("Report local danger; the source comparison is still incomplete") });
 }
 else if (S.bP13 && S.SC12.bPatrolRead && !S.SC12.bRouteReadback)
 {
  V.Choices.Add(PendingReadback == 2 ? S47Panels::FTalkChoice{ TEXT("ack-route"), TEXT("Agree 10.4; carry originals, take no reading at C") }
   : S47Panels::FTalkChoice{ TEXT("request-courier"), TEXT("Read the patrol time and the map; ask for a confirmed handover") });
 }
 V.Choices.Add({ TEXT("bye"), TEXT("Hang up") }); return V;
}

bool US47Chapter5::ChooseSC12(const FString& Id)
{
 if (Who == TEXT("sources"))
 {
  if (Id == TEXT("read0") || Id == TEXT("read1") || Id == TEXT("read2")) { ReadSource(FCString::Atoi(*Id.Right(1))); return true; }
  if (Id == TEXT("map")) { if (M && M->UI) M->UI->Close(true); OpenMap(); return true; }
  if (!HasSC12Sources() || S.SC12.ReadMask != 7) return true;
  if (Id == TEXT("origins"))
  {
   S.SC12.CheckMask |= 1;
   Lines = { Line(TEXT(""), TEXT("SARO: the original receiver run and the room note written during the escape. Hondo: its battery-backed logger and separate optical heads. Site 11: its own receiver chart and transit sheet. The three SOURCE IDs and local measurements differ; these are not three copies of one telex.")) };
  }
  if (Id == TEXT("clocks"))
  {
   if (!TextOf(Source(Host, 1)).Contains(TEXT("04:56:55")) || !TextOf(Source(Host, 2)).Contains(TEXT("04:56:56")))
   { Lines = { Line(TEXT(""), TEXT("The separate boundary and clock records are incomplete. Do not fill in a missing time.")) }; return true; }
   S.SC12.CheckMask |= 2;
   Lines = { Line(TEXT(""), TEXT("Hondo: 04:56:08, 04:56:55, 04:57:42. Site 11: 04:56:08, 04:56:56, 04:57:42. The middle boundaries differ by one second; checked local clocks explain the resolution, not perfect simultaneity. The optical readings at 05:00, 05:00:30 and 05:01 show decreasing ranges at each station. Same phase order, different local measurements. No arrival minute or global fleet count follows.")) };
  }
  if (Id == TEXT("feeds"))
  {
   S.SC12.CheckMask |= 4;
   Lines = { Line(TEXT(""), TEXT("Site 11's intersite audio feed was disconnected for the second observation. Its transit continued recording. Hondo's local power and optical sheet are separate from SARO's receiver and blackout. Repeated words could be a shared recording; the independently changing optical measurements cannot be established by the words alone.")) };
  }
  if (Id == TEXT("compare") && S.SC12.CheckMask == 7)
  {
   S.SC12.bCompared = true; File(SourceCheckDoc());
   Note(FString::Printf(TEXT("%s. Three distinct local sources compared: SARO07-860414-RX01, HONDO-860414-LOG01, SITE11-860414-TR01. Same reference order, differing local measurements and a one-second middle-boundary difference. Supported warning: coordinated preparation and continuing approach; identity and purpose unknown."), *Time()));
   Lines = { Line(TEXT(""), SourceCheckDoc().Page) };
   if (Host) Host->AdvanceClockTo(DeputyArrival, TEXT("SC12 independent source comparison completed"));
   Milestone();
  }
  return true;
 }
 if (Who != TEXT("ward")) return false;
 if (Id == TEXT("local-warning")) Lines = { Line(TEXT("E. WARD"), TEXT("The local hold is already in force. Stay inside the diner. Keep comparing the original sheets; do not claim a network result from a missing source.")) };
 if (Id == TEXT("send-warning") && SourcesVerified() && !S.SC12.bWarningSent)
 {
  PendingReadback = 1;
  Lines = { Line(TEXT("YOU"), FString::Printf(TEXT("Three independent local records: %s; %s; %s. First boundary: %s. Same order, different local readings. Site 11's middle boundary is one second later. The optical tracks continue to close."), SourceIds[0], SourceIds[1], SourceIds[2], FirstBoundary)),
   Line(TEXT("E. WARD"), FString::Printf(TEXT("Readback: %s; %s; %s. First boundary %s. I have the difference and the limits. I am entering this in my dated log outside SARO. Is that correct?"), SourceIds[0], SourceIds[1], SourceIds[2], FirstBoundary)) };
 }
 if (Id == TEXT("ack-warning") && PendingReadback == 1 && SourcesVerified())
 {
  PendingReadback = 0; S.SC12.bWarningSent = true; S.bWard = true; S.SC12.WarningStamp = Time(); File(WarningDoc());
  Lines = { Line(TEXT("YOU"), TEXT("Correct.")), Line(TEXT("E. WARD"), TEXT("Warning received. The 06:00 series remains suspended. No new observer closes a reference. I have asked the other sites to acknowledge. Keep the photographs and complete tables in the folder; you have not sent me the pictures. The courier has the field map. Check yours before we agree a bolt.")) };
  Note(FString::Printf(TEXT("%s. Ward read back the three source IDs and 04:56:08 MST, 14 APR 1986, then acknowledged the warning in her dated external log. The 06:00 hold continues. Originals and photographs remain in the folder."), *Time())); Milestone();
 }
 if (Id == TEXT("request-courier") && S.SC12.bWarningSent && S.SC12.bPatrolRead && S.bP13 && !S.SC12.bRouteReadback)
 {
  PendingReadback = 2;
  Lines = { Line(TEXT("YOU"), TEXT("The deputy's sheet is dated today, 04:15 MST. One clear drive on the north and old Roswell roads. On E14, transit on STATION 01, north to north: C goes east to 8.15. The handover bolt beyond it is 10.4.")),
   Line(TEXT("E. WARD"), TEXT("Readback: patrol at 04:15, an hour before he arrived. One report, not a map of everywhere it is safe. C at eight point one five. The courier is confirmed at ten point four, beyond C, and will wait there. Carry the originals; no stopping or new reading at C. The warning is already outside SARO. You can stay at the diner. This handover keeps the originals together; it is not a new experiment.")) };
 }
 if (Id == TEXT("ack-route") && PendingReadback == 2 && S.SC12.bWarningSent && S.SC12.bPatrolRead && S.bP13)
 {
  PendingReadback = 0; S.SC12.bCourier = S.SC12.bRouteReadback = true; S.SC12.RouteStamp = Time(); File(RouteDoc());
  Lines = { Line(TEXT("YOU"), TEXT("Ten point four. Past C without a reading. The originals stay together.")), Line(TEXT("E. WARD"), TEXT("I have the route and the patrol time in the log. If the truck falters before the crossing, turn back while you can. Nobody has promised a safe road.")) };
  Note(FString::Printf(TEXT("%s. Ward confirmed the courier at 10.4, beyond C at 8.15, and logged the 04:15 patrol report. No stopping or reading at C. The diner remains a refuge; the originals still have control value."), *Time()));
  UpdateRoadReady(); Milestone();
 }
 return true; // Unknown or out-of-order Ward choices never fall through to the legacy morning-series promise.
}

FS47Doc US47Chapter5::SourceCheckDoc() const
{
 return Paper(TEXT("HR_NETWORK_CHECK"), TEXT("Three sources / comparison and limits"), TEXT("14 APR 1986 / MST"),
  TEXT("SARO07-860414-RX01 / original receiver run and separately written room observation. HONDO-860414-LOG01 / local battery-backed event logger and independent optical heads. SITE11-860414-TR01 / local receiver and transit record, intersite audio feed disconnected during the second observation.\n\nHondo boundaries: 04:56:08, 04:56:55, 04:57:42. Site 11 boundaries: 04:56:08, 04:56:56, 04:57:42. Keep the one-second difference. The clocks were checked within one second; same phase order is not exact simultaneity.\n\nSeparate optical readings at 05:00, 05:00:30 and 05:01 show continuing approach. Matching recorded speech alone proves none of this. Supported working warning: coordinated preparation for an arrival. Identity, purpose, global count and an arrival minute remain unknown."));
}
FS47Doc US47Chapter5::WarningDoc() const
{
 return Paper(TEXT("HR_WARNING_RECEIPT"), TEXT("Ward / dated warning receipt"), S.SC12.WarningStamp + TEXT(" MST / 14 APR 1986"),
  FString::Printf(TEXT("ACKNOWLEDGED %s MST / 14 APR 1986 / E. WARD\n\nRead back and confirmed: %s; %s; %s. First boundary: %s. Different local measurements and the Site 11 middle-boundary difference retained. Entered in Ward's dated call log outside SARO.\n\nThe 06:00 morning series remains suspended. No observer is to close a reference. Other sites asked to acknowledge. This does not claim external activity has stopped.\n\nPhotographs, negatives, raw tables and complete originals remain in Reyes's transfer folder. The telephone sent no photographs. Courier location not agreed by this receipt."), *S.SC12.WarningStamp, SourceIds[0], SourceIds[1], SourceIds[2], FirstBoundary));
}
FS47Doc US47Chapter5::PatrolDoc() const
{
 return Paper(TEXT("HR_SC12_PATROL_SHEET"), TEXT("County sheriff / dated patrol sheet"), TEXT("14 APR 1986 / 04:15 MST"),
  TEXT("COUNTY SHERIFF / CALICHE SUBSTATION\nPATROL 14 APR 1986 / 04:15 MST\n\nNorth road and old Roswell road. One loop driven. No obstruction observed on the road at that time.\n\nThe deputy, at Mesa Diner from 05:15: 'Drove the loop at a quarter past four. Nothing on the road then. It is on the sheet.'\n\nOne drive, one hour before his arrival. This sheet promises nothing about conditions now, C, the lights, or every other road. It is not the 1985 tow ticket or the driver's old breakdown account."));
}
FS47Doc US47Chapter5::RouteDoc() const
{
 return Paper(TEXT("HR_ROUTE_READBACK"), TEXT("Ward / courier and route readback"), S.SC12.RouteStamp + TEXT(" MST / 14 APR 1986"),
  TEXT("Warning already receipted outside SARO. Dated patrol sheet read: 14 APR 1986, 04:15 MST.\n\nE09A transit on STATION 01, north up on E14. Follow C east to the crossing at 8.15. A and C are different. Continue beyond C to the confirmed courier at 10.4. Carry the originals. Do not stop or take a reading at C.\n\nThe deputy reported one unobstructed drive, an hour before his 05:15 arrival. Ward entered its time and confirmed the courier. No safe-road guarantee follows. The diner remains a refuge; the handover preserves the originals for checking. Turn back before C if the truck falters."));
}

S47J::FObj US47Chapter5::SC12Readback() const
{
 auto O = S47J::New(); S47J::Set(O, TEXT("state"), S.SC12.Save()); S47J::Set(O, TEXT("stage"), S.Stage);
 S47J::Set(O, TEXT("hasThreeSources"), HasSC12Sources()); S47J::Set(O, TEXT("sourcesVerified"), SourcesVerified()); S47J::Set(O, TEXT("readyForRoad"), ReadyForRoad());
 S47J::Set(O, TEXT("pendingLiveReadback"), PendingReadback); S47J::Set(O, TEXT("p13"), S.bP13);
 S47J::Set(O, TEXT("sourceIds"), TArray<FString>{SourceIds[0], SourceIds[1], SourceIds[2]}); S47J::Set(O, TEXT("firstBoundary"), FirstBoundary);
 S47J::Set(O, TEXT("clock"), Host ? Host->Clock() : 0.);
 TArray<TSharedPtr<FJsonValue>> Props;
 for (const auto& P : SC12Props)
 {
  auto Row = S47J::New(); S47J::Set(Row, TEXT("id"), P.Key); S47J::Set(Row, TEXT("node"), P.Value.I); S47J::Set(Row, TEXT("shown"), P.Value.Shown());
  const FVector At = P.Value.WorldPosition(); S47J::Set(Row, TEXT("worldThree"), TArray<double>{At.X, At.Y, At.Z});
  if (const auto* N = M && M->Web ? M->Web->Node(P.Value.I) : nullptr) if (auto* C = Cast<UStaticMeshComponent>(N->Comp.Get()))
  { S47J::Set(Row, TEXT("component"), C->GetPathName()); S47J::Set(Row, TEXT("visible"), C->IsVisible()); S47J::Set(Row, TEXT("mesh"), GetPathNameSafe(C->GetStaticMesh().Get())); }
  Props.Add(MakeShared<FJsonValueObject>(Row));
 }
 O->SetArrayField(TEXT("props"), Props);
 const FS47Town* Town = M && M->TownLayer ? M->TownLayer->Town.Get() : nullptr;
 S47J::Set(O, TEXT("deputyAtOldOffice"), Town && Town->bDeputyIn); S47J::Set(O, TEXT("oldPatrolCarOut"), Town && Town->bCarOut);
 // Read the retained SC03 components too: the old office schedule alone is not proof that an
 // earlier physical deputy has disappeared. No component or legacy visibility is changed here.
 bool OldDeputyVisible = false; TArray<TSharedPtr<FJsonValue>> OldDeputy;
 if (M && M->Web)
 {
  TInlineComponentArray<UStaticMeshComponent*> Components; M->Web->GetComponents(Components);
  for (const auto* C : Components) if (C && C->GetName().StartsWith(TEXT("SC03Deputy")))
  {
   bool Visible = !M->Web->IsHidden();
   for (const USceneComponent* P = C; P; P = P->GetAttachParent()) Visible &= P->IsVisible() && !P->bHiddenInGame;
   auto Row = S47J::New(); S47J::Set(Row, TEXT("component"), C->GetPathName()); S47J::Set(Row, TEXT("visible"), C->IsVisible());
   S47J::Set(Row, TEXT("hiddenInGame"), C->bHiddenInGame); S47J::Set(Row, TEXT("visibleThroughParents"), Visible);
   S47J::Set(Row, TEXT("mesh"), GetPathNameSafe(C->GetStaticMesh().Get())); OldDeputy.Add(MakeShared<FJsonValueObject>(Row));
   OldDeputyVisible |= Visible;
  }
 }
 S47J::Set(O, TEXT("oldSC03DeputyVisible"), OldDeputyVisible); O->SetArrayField(TEXT("oldSC03DeputyComponents"), OldDeputy);
 return O;
}

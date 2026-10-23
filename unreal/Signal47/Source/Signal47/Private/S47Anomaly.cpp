#include "S47Anomaly.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Kit.h"
#include "S47Image.h"
#include "S47Album.h"
#include "S47Night.h"
#include "S47Evening.h"
#include "S47Timeline.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47Three.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/Base64.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"

namespace
{
 FString Stamp(double C)
 {
  return FString(C < 0 || S47Timeline::Day(C) >= 12 * 3600 ? TEXT("13 APR 1986 / ") : TEXT("14 APR 1986 / ")) + S47Text::ClockText(C);
 }
 S47Kit::FW PaperPane(const FString& Title, const FString& Text)
 {
  return S47Kit::Col({S47Kit::Sub(Title), SNew(SBox).HeightOverride(340)[SNew(SScrollBox) + SScrollBox::Slot()[S47Kit::Paper(Text)]]});
 }
}

double FS47Anomaly::Clock() const { return M && M->Story ? M->Story->Clock() : 0; }
void FS47Anomaly::Begin(const S47J::FObj& Saved)
{
 Data = FS47AnomalyData(); Feedback.Empty();
 ArchiveUrl = S47J::Str(S47J::Obj(Saved,TEXT("archive")),TEXT("url"));
 bArchiveUnavailable = S47J::Bool(S47J::Obj(Saved,TEXT("archive")),TEXT("unavailable"));
#define LOAD_BOOL(Name) Data.Name = S47J::Bool(Saved, TEXT(#Name));
 LOAD_BOOL(bEarlyCamera) LOAD_BOOL(bLegacyCutout) LOAD_BOOL(bRoutineTimeKnown) LOAD_BOOL(N01Routine) LOAD_BOOL(N01Compared) LOAD_BOOL(N01Followed)
 LOAD_BOOL(N02Observed) LOAD_BOOL(N02Control) LOAD_BOOL(N02Compared)
 LOAD_BOOL(A02Moved) LOAD_BOOL(A02Observed) LOAD_BOOL(A02Compared) LOAD_BOOL(A02Followed)
 LOAD_BOOL(P01Pending) LOAD_BOOL(P01Heard) LOAD_BOOL(P01Followed) LOAD_BOOL(bDatedReference)
#undef LOAD_BOOL
#define LOAD_STRING(Name) Data.Name = S47J::Str(Saved, TEXT(#Name));
 LOAD_STRING(ReferencePhoto) LOAD_STRING(ChangedPhoto) LOAD_STRING(N02Photo) LOAD_STRING(ControlPhoto)
 LOAD_STRING(N01Assessment) LOAD_STRING(N01Choice) LOAD_STRING(N02Assessment) LOAD_STRING(A02Assessment)
#undef LOAD_STRING
#define LOAD_TIME(Name) Data.Name = S47J::Num(Saved, TEXT(#Name));
 LOAD_TIME(RoutineClock) LOAD_TIME(ReferenceClock) LOAD_TIME(ChangedClock)
 LOAD_TIME(N02Clock) LOAD_TIME(N02PhotoClock) LOAD_TIME(ControlClock) LOAD_TIME(P01Clock)
#undef LOAD_TIME
 FString Error; bScriptReady = Script.Initialize(Error);
 if (bScriptReady)
 {
  if (S47J::Obj(Saved, TEXT("script")))
  {
   if (!Script.Load(S47J::Obj(Saved, TEXT("script")), Error)) UE_LOG(LogTemp, Warning, TEXT("S47: anomaly script restore: %s"), *Error);
  }
  else
  {
   // En gammel sak får dokumenttilgang, aldri oppdiktede fotografier eller tidlig arbeid.
   auto State = Script.Save(); auto Flags = S47J::Obj(State, TEXT("flags"));
   S47J::Set(Flags, TEXT("handover_done"), true);
   S47J::Set(Flags, TEXT("routine_checked"), Data.N01Routine);
   Script.Load(State, Error);
  }
 }
 if (M && M->Story && !Saved && (!M->Story->bEveningTimeline || M->Story->ChapterKey != TEXT("prologue")))
 {
  // Alternativet er uttrykkelig en datert arkivkilde, ikke spillerens eksponering.
  Data.bDatedReference = true;
 }
 if (!Saved && M && M->TownSeen.Contains(TEXT("ev:cutout"))) Data.bLegacyCutout=true;
 Sync(); Refile();
}

S47J::FObj FS47Anomaly::Save() const
{
 auto O = S47J::New(); S47J::Set(O, TEXT("version"), 1);
#define SAVE_BOOL(Name) S47J::Set(O, TEXT(#Name), Data.Name);
 SAVE_BOOL(bEarlyCamera) SAVE_BOOL(bLegacyCutout) SAVE_BOOL(bRoutineTimeKnown) SAVE_BOOL(N01Routine) SAVE_BOOL(N01Compared) SAVE_BOOL(N01Followed)
 SAVE_BOOL(N02Observed) SAVE_BOOL(N02Control) SAVE_BOOL(N02Compared)
 SAVE_BOOL(A02Moved) SAVE_BOOL(A02Observed) SAVE_BOOL(A02Compared) SAVE_BOOL(A02Followed)
 SAVE_BOOL(P01Pending) SAVE_BOOL(P01Heard) SAVE_BOOL(P01Followed) SAVE_BOOL(bDatedReference)
#undef SAVE_BOOL
#define SAVE_STRING(Name) S47J::Set(O, TEXT(#Name), Data.Name);
 SAVE_STRING(ReferencePhoto) SAVE_STRING(ChangedPhoto) SAVE_STRING(N02Photo) SAVE_STRING(ControlPhoto)
 SAVE_STRING(N01Assessment) SAVE_STRING(N01Choice) SAVE_STRING(N02Assessment) SAVE_STRING(A02Assessment)
#undef SAVE_STRING
#define SAVE_TIME(Name) S47J::Set(O, TEXT(#Name), Data.Name);
 SAVE_TIME(RoutineClock) SAVE_TIME(ReferenceClock) SAVE_TIME(ChangedClock)
 SAVE_TIME(N02Clock) SAVE_TIME(N02PhotoClock) SAVE_TIME(ControlClock) SAVE_TIME(P01Clock)
#undef SAVE_TIME
 if (bScriptReady) S47J::Set(O, TEXT("script"), Script.Save());
 if (!ArchiveUrl.IsEmpty())
 {
  auto Archive=S47J::New();
  S47J::Set(Archive,TEXT("id"),TEXT("anomaly-archive-display-1710"));
  S47J::Set(Archive,TEXT("url"),ArchiveUrl);
  S47J::Set(Archive,TEXT("clock"),-24600.);
  if (bArchiveUnavailable) S47J::Set(Archive,TEXT("unavailable"),true);
  S47J::Set(O,TEXT("archive"),Archive);
 }
 return O;
}

const FS47AlbumShot* FS47Anomaly::Shot(const FString& Id) const
{
 if (Id.IsEmpty() || !M || !M->Night || !M->Night->Album) return nullptr;
 return M->Night->Album->Shots().FindByPredicate([&](const FS47AlbumShot& P){ return P.Id == Id; });
}
bool FS47Anomaly::ShotUnavailable(const FString& Id) const
{
 if (const auto* P=Shot(Id); P && P->bUnavailable) return true;
 if (!M || !M->Flow || Id.IsEmpty()) return false;
 for (const auto& V : S47J::Arr(S47J::Obj(M->Flow->Carry,TEXT("album")),TEXT("shots")))
 {
  if (!V.IsValid() || V->Type!=EJson::Object) continue;
  const auto O=V->AsObject();
  if (S47J::Str(O,TEXT("id"))==Id) return S47J::Bool(O,TEXT("unavailable"));
 }
 return false;
}
const FS47AlbumShot* FS47Anomaly::ReadyShot(const FString& Id) const
{
 const auto* P = Shot(Id);
 if (!P || P->bUnavailable || ShotUnavailable(Id) || S47Timeline::Night(Clock()) - S47Timeline::Night(P->Clock) < 90) return nullptr;
 TArray<FColor> Pixels; int32 W=0,H=0;
 return S47Image::DecodePixels(P->Url,Pixels,W,H) && W>0 && H>0 ? P : nullptr;
}
bool FS47Anomaly::HasPrintout() const
{
 return M && M->Story && M->Story->Docs().ContainsByPredicate([](const FS47Doc& D){return D.Id == TEXT("printout");});
}
void FS47Anomaly::Log(const FString& Event) const
{
 UE_LOG(LogTemp, Display, TEXT("S47: anomaly %s clock=%.3f ref=%s changed=%s N01=%s N02=%s A02=%s"), *Event, Clock(), *Data.ReferencePhoto, *Data.ChangedPhoto, *Data.N01Assessment, *Data.N02Assessment, *Data.A02Assessment);
}
void FS47Anomaly::Changed()
{
 Sync(); Refile();
 if (M) M->RequestAutosave();
}
void FS47Anomaly::Sync()
{
 if (M && M->Evening && M->Evening->Enabled())
 {
  auto& P = M->Evening->Pack;
  Data.N01Routine |= P.Flag(TEXT("routine_checked"));
  // Disse flaggene kommer bare fra aktivt valgte authored scener.
  if (P.Flag(TEXT("record_compared")) && !Data.N01Compared)
  { Data.N01Compared = true; Data.N01Assessment = TEXT("unresolved"); }
 }
}
void FS47Anomaly::Step(double Dt)
{
 const bool Before = Data.N01Routine;
 const bool Compared = Data.N01Compared;
 const bool Dated = Data.bDatedReference;
 // N01s spillerpanel eier den lagrede, lokale vurderingen. Speil bare et ferdig
 // resultat til kveldspakkens porter; Evening skal aldri gjenåpne feilvalget.
 if (M && M->Evening && M->Evening->Enabled() && Data.N01Compared && HasPrintout())
 {
  auto& P=M->Evening->Pack;
  if (!P.CurrentScene() && P.Flag(TEXT("routine_checked")) && !P.Flag(TEXT("record_compared")))
  {
   P.SetExternal(TEXT("ctx_signal_present"),true);
   if (P.SetPhase(TEXT("signal")) && P.Start(TEXT("N01_COMPARE")))
   {
    CompleteScene(P);
    if (P.Select(Data.N01Choice==TEXT("insufficient")?TEXT("N01_CH_COMPARE_UNCERTAIN"):TEXT("N01_CH_COMPARE_EARLIER"))) CompleteScene(P);
    if (M) M->RequestAutosave();
   }
  }
 }
 if (M && M->Evening && M->Evening->Enabled() && M->Story)
 {
  auto& P=M->Evening->Pack;
  const bool Quiet=!M->ModalOpen() && !M->Story->IsCinematic() && !M->OneAtATime();
  const bool AllNight=M->Story->ChapterKey==TEXT("ch5");
  P.SetExternal(TEXT("ctx_room_quiet"),Quiet);
  P.SetExternal(TEXT("ctx_all_night_active"),AllNight);
  bool Mirrored=false;
  // Bare etterprøvde lokale kontroller speiles som funn. En gyldig uavklart
  // avslutning gir fremdrift i piloten, men utgir seg aldri for et bekreftet avvik.
  if (!P.CurrentScene() && Quiet && Data.N02Compared && Data.N02Control &&
      Data.N02Assessment==TEXT("ordinary lamp silhouette") && !P.Flag(TEXT("field_reflection_checked")))
  {
   if (P.SetPhase(TEXT("early_shift")) && P.Start(TEXT("N02_REFLECTION")))
   { CompleteScene(P); Mirrored=true; }
  }
  if (!P.CurrentScene() && AllNight && Data.A02Compared && !Data.bLegacyCutout &&
      Data.A02Assessment==TEXT("same object, changed position; cause unconfirmed") &&
      !P.Flag(TEXT("field_cutout_compared")))
  {
   if (P.SetPhase(TEXT("all_night")) && P.Start(TEXT("A02_COMPARE")))
   {
    CompleteScene(P);
    if (P.Select(TEXT("A02_CH_MOVED"))) { CompleteScene(P); Mirrored=true; }
   }
  }
  if (!P.CurrentScene() && AllNight && Quiet && Data.P01Heard && !Data.bLegacyCutout &&
      P.Flag(TEXT("field_cutout_compared")) && !P.Flag(TEXT("field_studio_after_heard")))
  {
   if (P.SetPhase(TEXT("all_night")) && P.Start(TEXT("P01_STUDIO_AFTER")))
   { CompleteScene(P); Mirrored=true; }
  }
  if (Mirrored) M->RequestAutosave();
 }
 if (M && M->Story && !Data.bDatedReference)
 {
  Data.bDatedReference = M->Story->Docs().ContainsByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_TOWN_REFERENCE");});
 }
 Sync(); if (Before != Data.N01Routine || Compared != Data.N01Compared || Dated != Data.bDatedReference) { Refile(); if (M) M->RequestAutosave(); }
}

FS47Doc FS47Anomaly::ReferenceDoc() const
{
 FS47Doc D;
 D.Id = TEXT("anomaly:reference"); D.Title = TEXT("Caliche display / dated reference"); D.Kind = TEXT("typed");
 D.Stamp = TEXT("SARO HANDOVER COPY / SUNDAY 13 APRIL 1986");
 D.Page = TEXT("CALICHE SOUVENIR DISPLAY / DAY OPERATOR'S INVENTORY\n\nCardboard figure in the souvenir-shop window. Yellow price sticker: $3.50, at the lower right. Split at the lower left edge. Record position against the shop window and pavement.\n\nFiled by the day operator with Sunday's local service copies. This is a dated written reference, not a photograph taken by the night operator. It records the display identity and position. It is not a record of the night operator's visit.\n\nFor a stronger control, photograph the actual display from the marked pavement position while it remains in the window.");
 D.Transcript = D.Page;
 // Arkivkopien er et faktisk runtime-opptak levert med piloten. Den må kunne dekodes.
 // Bevar lagret bildereferanse når kildens nåværende versjon senere byttes ut.
 D.Image=ArchiveUrl;
 if (D.Image.IsEmpty() && M && M->Story) for (const auto& Old : M->Story->Docs())
  if (Old.Id == D.Id && !Old.Image.IsEmpty()) { D.Image = Old.Image; break; }
 if (D.Image.IsEmpty())
 {
  TArray<uint8> Bytes;
  if (FFileHelper::LoadFileToArray(Bytes, *(FPaths::ProjectContentDir() / TEXT("Anomaly/ReferenceWindow.jpg"))))
   D.Image = TEXT("data:image/jpeg;base64,") + FBase64::Encode(Bytes);
 }
 if (!D.Image.IsEmpty())
 {
  ArchiveUrl=D.Image;
  D.Title = TEXT("Caliche display / archived reference print");
  D.Stamp = TEXT("DAY OPERATOR / 13 APR 1986 / 17:10");
  D.Page = TEXT("ARCHIVED DISPLAY PRINT / 13 APRIL 1986, 17:10\n\nThe day operator's reference print, filed with the Sunday handover. This is not a photograph taken by the night operator. The cardboard figure is in the souvenir-shop window. Compare the yellow $3.50 sticker, the split lower left edge and the window position.\n\nThe print records the ordinary display identity and position for the shift file."); D.Transcript=D.Page;
  if (bArchiveUnavailable)
  {
   D.Page+=TEXT("\n\nPRINT UNAVAILABLE / the saved original could not be read. Its replacement card is not photographic identity evidence. Use insufficient evidence or another actual reference."); D.Transcript=D.Page;
  }
 }
 return D;
}
void FS47Anomaly::File(const FString& Id, const FString& Title, const FString& Page)
{
 if (!M || !M->Story) return;
 FS47Doc D; D.Id = Id; D.Title = Title; D.Kind = TEXT("typed"); D.Page = Page; D.Transcript = Page;
 D.Stamp = TEXT("FIELD CHECK / REVISABLE ASSESSMENT"); M->Story->AddDoc(D);
}
void FS47Anomaly::Refile()
{
 if (!M || !M->Story) return;
 const bool Available = !M->Evening || !M->Evening->Enabled() || M->Evening->Pack.Flag(TEXT("handover_done"));
 if (Available || Data.bDatedReference) M->Story->AddDoc(ReferenceDoc());
 if (Data.N01Routine)
 {
  for (const TCHAR* Id : {TEXT("EV_SERVICE_LOG"), TEXT("EV_RADIO_SCHEDULE")})
   if (const auto* P = Script.FindDocument(Id))
   {
    // Bevar dokumentet som faktisk ble hentet eller lastet, også i eldre saker.
    if (M->Story->Docs().ContainsByPredicate([&](const FS47Doc& D){ return D.Id == Id; })) continue;
    FS47Doc D; D.Id=P->Id; D.Title=P->Title; D.Kind=P->Kind; D.Stamp=P->Stamp; D.Page=FString::Join(P->Body,TEXT("\n\n")); D.Transcript=D.Page; M->Story->AddDoc(D);
   }
  FString P = TEXT("REFERENCE / service copy and transmission schedule read at ") + (Data.bRoutineTimeKnown?Stamp(Data.RoutineClock):TEXT("a time not recorded in the restored case")) + TEXT(".\n\nThe service complaint predates this shift. The schedule establishes planned programmes, not actual transmissions.");
  if (Data.N01Compared) P += TEXT("\n\nASSESSMENT / ") + Data.N01Assessment + TEXT(". Earlier complaint; later residual. A shared cause is not established.");
  if (Data.N01Followed) P += TEXT("\n\nFOLLOW-UP / inspection result requested for the shift file. A request is not an inspection result.");
  else if (Data.N01Compared) P += TEXT("\n\nNEXT / leave a request for the actual inspection result with the handover folder.");
  File(TEXT("anomaly:n01"), HasPrintout()?TEXT("N01 / Service records and residual"):TEXT("N01 / Service references"), P);
 }
 if (Data.N02Observed || !Data.N02Photo.IsEmpty())
 {
  FString P = TEXT("OBSERVATION / pale outline at the Caliche laundromat. First noted at ") + Stamp(Data.N02Clock) + TEXT(".\n\nEVIDENCE / first frame: ") + (Data.N02Photo.IsEmpty()?TEXT("not taken"):Data.N02Photo) + TEXT("; side control: ") + (Data.ControlPhoto.IsEmpty()?TEXT("not taken"):Data.ControlPhoto) + TEXT(".");
  if (Data.N02Compared) P += TEXT("\n\nASSESSMENT / ") + Data.N02Assessment + TEXT(".");
  P += Data.N02Assessment==TEXT("ordinary lamp silhouette") ? TEXT(" The outline follows the ordinary worklamp when viewed from the side. No independent person established.") : TEXT("\n\nNEXT / photograph the bright outline, move to the side and inspect the lamp. Photograph that control, then compare position.");
  File(TEXT("anomaly:n02"), TEXT("N02 / Lamp and outline"), P);
 }
 if (Data.bDatedReference || !Data.ReferencePhoto.IsEmpty() || Data.A02Observed || !Data.ChangedPhoto.IsEmpty())
 {
  const bool Encountered=Data.A02Observed || !Data.ChangedPhoto.IsEmpty();
  FString P = Data.ReferencePhoto.IsEmpty() ? TEXT("REFERENCE / dated Sunday archive reference, filed by the day operator. No personal reference photograph.") : TEXT("REFERENCE / photograph ") + Data.ReferencePhoto + TEXT(" / ") + Stamp(Data.ReferenceClock) + TEXT(".");
  if (Data.bLegacyCutout) P += TEXT("\n\nRESTORED HISTORY / a cutout event was already recorded in the original case. It will not be replayed or converted into a newly verified comparison. Use the service records or lamp control as another investigation.");
  if (Encountered)
  {
   P += TEXT("\n\nLATER EVIDENCE / ") + (Data.ChangedPhoto.IsEmpty()?TEXT("no photograph attached"):Data.ChangedPhoto + TEXT(" / ") + Stamp(Data.ChangedClock)) + TEXT(".");
   if (Data.A02Compared) P += TEXT("\n\nASSESSMENT / ") + Data.A02Assessment + TEXT(".");
   if (!Data.bLegacyCutout) P += Data.A02Followed ? TEXT("\n\nFOLLOW-UP / price sticker and split edge inspected in place. Same cardboard construction; no observation of who moved it.") : TEXT("\n\nNEXT / compare the price sticker, torn corner and position. Then inspect those details on the figure.");
  }
  else P+=TEXT("\n\nROUTINE REFERENCE / keep the souvenir display in the shift file. Include the yellow price sticker, split edge and window position in a photograph.");
  File(TEXT("anomaly:a02"), Encountered?TEXT("A02 / Same figure, changed position"):TEXT("Display reference"), P);
 }
 if (Data.P01Heard)
 {
  File(TEXT("anomaly:p01"), TEXT("P01 / Printer rhythm at the studio"), TEXT("OBSERVATION / ") + Stamp(Data.P01Clock) + TEXT(". A short printer rhythm from the direction of the closed studio. No conversation and no figure observed.\n\nCONTROL / the transmission schedule ends Earl's Night Line at 04:00. An empty studio after four is normal. A schedule cannot identify the source of this sound.") + (Data.P01Followed?TEXT("\n\nFOLLOW-UP / facade inspected. No printer or person visible. The source remains unresolved."):TEXT("\n\nNEXT / approach the studio window and inspect the ordinary facade. The sound alone does not establish a presence.")));
 }
}

FString FS47Anomaly::SceneText(const FString& Id) const
{
 TArray<FString> Text;
 if (const auto* S = Script.FindScene(Id)) for (const auto& L : S->Lines) Text.Add(L.Text);
 return FString::Join(Text, TEXT("\n\n"));
}
FString FS47Anomaly::CompleteScene(FS47EveningPack& Pack)
{
 TArray<FString> Text;
 // Bare tekstscener, aldri lyd/dialog. Hvert Advance bekrefter en synlig linje.
 for (int32 I=0; I<20 && Pack.CurrentScene(); ++I)
 {
  const auto* S = Pack.CurrentScene(); if (S->Channel != TEXT("ui")) break;
  if (const auto* L = Pack.CurrentLine()) Text.Add(L->Text);
  if (!Pack.Advance()) break;
  if (Pack.CurrentScene() && !Pack.CurrentLine() && !Pack.Choices().IsEmpty()) break;
 }
 return FString::Join(Text, TEXT("\n\n"));
}
FS47EveningPack& FS47Anomaly::Narrative(bool bRoutine)
{
 // Den delte kveldspakken eier bare rutinen. Lokal Script bevarer alle
 // mellomstadier i sammenligningen, inkludert et feilvalg som kan korrigeres.
 if (bRoutine && M && M->Evening && M->Evening->Enabled())
 {
  auto& P = M->Evening->Pack;
  const auto* Active = P.CurrentScene();
  if (Active && Active->Id==TEXT("N01_ROUTINE")) return P;
  if (!Active && P.Flag(TEXT("handover_done")) && !Data.N01Routine &&
      Clock() < S47Timeline::Residual(M->Story->bEveningTimeline)) return P;
 }
 return Script;
}
void FS47Anomaly::Routine()
{
 if (!M || !M->Story || Data.N01Routine) return;
 const bool Early = Clock() < S47Timeline::Residual(M->Story->bEveningTimeline) && !HasPrintout();
 if (Early)
 {
  auto& P = Narrative(true);
  if (!(P.CurrentScene() && P.CurrentScene()->Id==TEXT("N01_ROUTINE")) && (!P.SetPhase(TEXT("early_shift")) || !P.Start(TEXT("N01_ROUTINE"))))
  { Feedback=TEXT("Finish the current call before checking the folder."); Open(TEXT("n01")); return; }
  Feedback = CompleteScene(P);
 }
 else Feedback = TEXT("Preserved Sunday copies read now. This does not record an earlier visit or create a reference photograph.");
 Data.N01Routine = true; Data.RoutineClock = Clock(); Data.bRoutineTimeKnown=true; Changed(); Log(TEXT("N01 documented")); Open(TEXT("n01"));
}
bool FS47Anomaly::StartComparison(FS47EveningPack& P)
{
 if (P.CurrentScene())
 {
  if (!P.CurrentScene()->Id.StartsWith(TEXT("N01_COMPARE"))) return false;
  CompleteScene(P); return true;
 }
 // Den lokale adapteren gjenåpner en vurdering uten å viske ut sakens bevis.
 if (&P == &Script)
 {
  auto State=P.Save(); auto Flags=S47J::Obj(State,TEXT("flags"));
  S47J::Set(Flags,TEXT("routine_checked"),Data.N01Routine);
  S47J::Set(Flags,TEXT("record_compared"),false);
  FString Error; if (!P.Load(State,Error)) return false;
 }
 P.SetExternal(TEXT("ctx_signal_present"), HasPrintout());
 if (!P.SetPhase(TEXT("signal")) || !P.Start(TEXT("N01_COMPARE"))) return false;
 CompleteScene(P); return true;
}

bool FS47Anomaly::Assess(const FString& InId, const FString& Choice)
{
 const FString Id = InId.Replace(TEXT("anomaly:"), TEXT(""));
 if (Id == TEXT("n01"))
 {
  if (!Data.N01Routine || !HasPrintout()) { Feedback=TEXT("Keep the dated service copy, schedule and actual residual printout together before comparing."); return false; }
  auto& P=Narrative(); if (!StartComparison(P)) { Feedback=TEXT("Finish the current conversation before comparing the records."); return false; }
  FString Pick = Choice==TEXT("cause")?TEXT("N01_CH_COMPARE_CAUSE"):Choice==TEXT("insufficient")?TEXT("N01_CH_COMPARE_UNCERTAIN"):TEXT("N01_CH_COMPARE_EARLIER");
  if (!P.Select(Pick))
  {
   // Gjenåpnet feilvurdering viser samme konkrete korreksjon og revisjonsvalg.
   if (Choice == TEXT("revise") && P.Select(TEXT("N01_CH_REVISE"))) { Feedback=CompleteScene(P); Data.N01Compared=true; Data.N01Assessment=TEXT("unresolved"); Data.N01Choice=TEXT("insufficient"); }
   else return false;
  }
  else
  {
   Feedback=CompleteScene(P);
   if (Choice==TEXT("cause")) { Data.N01Assessment=TEXT("correction needed: no causal control"); Changed(); return false; }
   Data.N01Compared=true; Data.N01Assessment=TEXT("unresolved"); Data.N01Choice=Choice==TEXT("insufficient")?TEXT("insufficient"):TEXT("earlier");
  }
 }
 else if (Id == TEXT("n02"))
 {
  if (Choice==TEXT("reflection") && Data.N02Control && ReadyShot(Data.N02Photo) && ReadyShot(Data.ControlPhoto))
  { Data.N02Compared=true; Data.N02Assessment=TEXT("ordinary lamp silhouette"); Feedback=TEXT("The two views expose the upright board and ordinary worklamp. The silhouette changes with the viewing position; no independent person established."); }
  else if (Choice==TEXT("insufficient"))
  { Data.N02Compared=true; Data.N02Assessment=TEXT("insufficient evidence"); Feedback=TEXT("Filed as unresolved. Keep the first frame, change position and photograph the lamp control. You can revise this entry."); }
  else { Feedback=TEXT("Two developed views and the side-position lamp control are needed. Each instant print needs 90 seconds outside the paused panel. An outline alone cannot establish an independent person."); return false; }
 }
 else if (Id == TEXT("a02"))
 {
  if (Choice==TEXT("living")) { Feedback=SceneText(TEXT("A02_CAUTION")); return false; }
  if (Choice==TEXT("moved") && ReadyShot(Data.ChangedPhoto) && (ReadyShot(Data.ReferencePhoto) || (Data.bDatedReference && !bArchiveUnavailable && S47Image::FromDataUrl(ReferenceDoc().Image))))
  { Data.A02Compared=true; Data.A02Assessment=TEXT("same object, changed position; cause unconfirmed"); Feedback=SceneText(TEXT("A02_FILED")); }
  else if (Choice==TEXT("insufficient"))
  { Data.A02Compared=true; Data.A02Assessment=TEXT("insufficient evidence"); Feedback=TEXT("Filed as unresolved. Keep any missing frame with this entry if you obtain it later. The diner records are still worth checking."); }
  else { Feedback=TEXT("Attach a developed later photograph and either your developed earlier frame or the archived display print. Each instant print needs 90 seconds outside the paused panel. A note about a silhouette does not establish identity."); return false; }
  Data.P01Pending = !Data.P01Heard && !Data.bLegacyCutout;
 }
 else return false;
 Changed(); Log(Id + TEXT(" assessed")); return true;
}

void FS47Anomaly::Photo(const FS47AlbumShot& P)
{
 bool Added=false;
 auto Target=[&](const TCHAR* Id){ return P.Targets.ContainsByPredicate([&](const TPair<FString,FVector2D>& T){return T.Key==Id;}); };
 if (Target(TEXT("anomaly:reference")) && Data.ReferencePhoto.IsEmpty()) { Data.ReferencePhoto=P.Id; Data.ReferenceClock=P.Clock; Added=true; }
 if (Target(TEXT("anomaly:changed")) && Data.ChangedPhoto.IsEmpty()) { Data.ChangedPhoto=P.Id; Data.ChangedClock=P.Clock; Data.A02Observed=true; Added=true; }
 if (Target(TEXT("anomaly:n02reference")) && Data.N02Photo.IsEmpty()) { Data.N02Photo=P.Id; Data.N02PhotoClock=P.Clock; if (!Data.N02Observed) Data.N02Clock=P.Clock; Data.N02Observed=true; Added=true; }
 if (Target(TEXT("anomaly:n02control")) && Data.ControlPhoto.IsEmpty()) { Data.ControlPhoto=P.Id; Data.ControlClock=P.Clock; Added=true; }
 if (Added)
 {
  Changed(); Log(TEXT("photograph linked"));
  if (M && M->UI) M->UI->Toast(TEXT("Frame attached to the field check. Compare it in the notebook; the exposure alone is not a conclusion."),5);
 }
}

bool FS47Anomaly::Use(const FString& Id)
{
 if (Id==TEXT("anomaly:routine")) { Routine(); return true; }
 if (Id==TEXT("anomaly:dated"))
 {
  Data.bDatedReference=true; Changed(); Log(TEXT("dated reference read")); Open(TEXT("reference")); return true;
 }
 if (Id==TEXT("anomaly:n01follow"))
 {
  if (Data.N01Compared)
  {
   if (!M || !M->Web || !M->Player || FVector::Dist(M->Player->Pos,S47Three::ToThree(M->Web->PropLocation(TEXT("logbook")))) > 3.5)
   {
    if (M && M->UI) M->UI->Toast(TEXT("Leave the request with the physical shift folder on the SARO control-room desk."),5);
    return true;
   }
   Data.N01Followed=true; Changed(); Log(TEXT("N01 follow-up request"));
   if (M && M->UI) M->UI->Toast(TEXT("Inspection-result request left in the shift folder. The supply explanation remains untested."),5);
  }
  return true;
 }
 if (Id==TEXT("anomaly:n02control"))
 {
  // Fysisk kontroll blir satt av World, som har ståsted og siktlinje.
  Open(TEXT("n02")); return true;
 }
 if (Id==TEXT("anomaly:changed")) { Open(TEXT("a02")); return true; }
 if (Id==TEXT("anomaly:reference")) { Open(TEXT("reference")); return true; }
 for (const TCHAR* K : {TEXT("n01"),TEXT("n02"),TEXT("a02"),TEXT("p01")})
  if (Id==FString(TEXT("anomaly:"))+K) { Open(K); return true; }
 return false;
}
bool FS47Anomaly::OpenDoc(const FS47Doc& D)
{
 if (!D.Id.StartsWith(TEXT("anomaly:"))) return false;
 if (D.Id==TEXT("anomaly:reference"))
 {
  Data.bDatedReference=true; Changed(); Log(TEXT("archived reference opened"));
 }
 Open(D.Id.Mid(8)); return true;
}
TArray<FS47Task> FS47Anomaly::Tasks() const
{
 TArray<FS47Task> T;
 if (!M || !M->Story) return T;
 if (M->Evening && M->Evening->Opening()) return T;
 T.Add({Data.N01Routine?TEXT("N01 / Keep the dated service copy and transmission schedule for comparison."):TEXT("N01 / Read the local service copy and transmission schedule in the SARO shift folder."),Data.N01Routine});
 if (HasPrintout()) T.Add({TEXT("N01 / Compare the earlier service records with the actual residual printout."),Data.N01Compared});
 if (Data.N01Compared) T.Add({TEXT("N01 / Leave an inspection-result request with the SARO shift folder."),Data.N01Followed});
 if (Data.N02Observed) T.Add({TEXT("N02 / Photograph the pale outline, move to the side and document the worklamp control. Compare both views."),Data.N02Compared});
 if (Data.A02Observed || !Data.ChangedPhoto.IsEmpty())
  T.Add({TEXT("A02 / Compare the display reference with a later photograph. Check price sticker, torn edge and position."),Data.A02Compared});
 else T.Add({TEXT("Keep a photograph of the souvenir display as a routine reference."),!Data.ReferencePhoto.IsEmpty()});
 if (Data.A02Compared && (Data.A02Observed || !Data.ChangedPhoto.IsEmpty()) && !Data.bLegacyCutout) T.Add({TEXT("A02 / Inspect the figure's sticker and split edge in place."),Data.A02Followed});
 if (Data.P01Heard) T.Add({TEXT("P01 / Inspect the closed studio facade. A sound alone does not identify its source."),Data.P01Followed});
 return T;
}

void FS47Anomaly::Open(const FString& InId)
{
 if (!M || !M->UI || !M->Story) return;
 FString Id=InId.Replace(TEXT("anomaly:"),TEXT(""));
 const bool FigureEncountered=Data.A02Observed || !Data.ChangedPhoto.IsEmpty();
 if (Id==TEXT("a02") && !FigureEncountered && !Data.bLegacyCutout) { Id=TEXT("reference"); Feedback.Empty(); }
 if ((Id==TEXT("n02") && !Data.N02Observed && Data.N02Photo.IsEmpty()) || (Id==TEXT("p01") && !Data.P01Heard))
 { Feedback.Empty(); Open(); return; }
 TArray<S47Kit::FW> R;
 if (Id==TEXT("n01") && Feedback.IsEmpty() && Data.N01Assessment.StartsWith(TEXT("correction needed")))
  Feedback=SceneText(TEXT("N01_COMPARE_WRONG"));
 FString Title=TEXT("FIELD CHECKS");
 if (!Feedback.IsEmpty()) R.Add(S47Kit::Paper(Feedback));
 auto Button=[this,&R,Id](const FString& Label,const FString& Choice)
 {
  R.Add(S47Kit::Button(Label,[this,Id,Choice]{Assess(Id,Choice); Open(Id);}));
 };
 auto Picture=[this](const FString& PhotoId,const FString& Label)->S47Kit::FW
 {
  TArray<S47Kit::FW> P; P.Add(S47Kit::Sub(Label));
  const auto* S=Shot(PhotoId);
  if (S)
  {
   P.Add(S47Kit::Small(S->Id + TEXT(" / ") + Stamp(S->Clock) + TEXT(" / ") + S->Place));
   if (ShotUnavailable(PhotoId)) P.Add(S47Kit::Paper(TEXT("PRINT UNAVAILABLE / the original saved image could not be read. Its metadata is retained; the replacement card cannot support an identity check. Use insufficient evidence.")));
   else if (S47Timeline::Night(Clock())-S47Timeline::Night(S->Clock)<90) P.Add(S47Kit::Paper(TEXT("PRINT DEVELOPING / allow 90 seconds after exposure. Time is paused in this panel; close it while the chemistry works.")));
   else if (ReadyShot(PhotoId)) P.Add(S47Kit::Image(S47Image::FromDataUrl(S->Url),410));
   else P.Add(S47Kit::Paper(TEXT("IMAGE UNAVAILABLE / the saved frame cannot be decoded. Its metadata is retained. Use insufficient evidence.")));
   P.Add(S47Kit::Small(S->Text));
  }
  else P.Add(S47Kit::Paper(TEXT("NO PHOTOGRAPH ATTACHED / take this view with the field camera. An observation note is not an exposure.")));
  return S47Kit::Col(P);
 };
 if (Id.IsEmpty())
 {
  Feedback.Empty();
  R.Add(S47Kit::Text(TEXT("Discover. Photograph or note what you actually observed. Put relevant sources side by side, choose a control, then follow up. Every assessment can be revised.")));
  R.Add(S47Kit::Card(TEXT("N01 / Service records"),HasPrintout()?TEXT("Earlier service complaint, actual later printout. Compare their dates and the limits of each record."):TEXT("Dated service copy and transmission schedule. Check the ordinary operating records."),[this]{Feedback.Empty();Open(TEXT("n01"));}));
  if (Data.N02Observed || !Data.N02Photo.IsEmpty()) R.Add(S47Kit::Card(TEXT("N02 / Pale outline"),TEXT("Two positions and a worklamp control. What changes when you move?"),[this]{Feedback.Empty();Open(TEXT("n02"));}));
  R.Add(S47Kit::Card(FigureEncountered?TEXT("A02 / Display figure"):TEXT("Display reference"),FigureEncountered?TEXT("Price sticker, torn edge and position. Compare the same object across two records."):TEXT("Keep the ordinary souvenir display, its price sticker and position in the shift file."),[this]{Feedback.Empty();Open(TEXT("a02"));}));
  if (Data.P01Heard) R.Add(S47Kit::Card(TEXT("P01 / Closed studio"),TEXT("A familiar printer rhythm. Inspect its apparent source without inventing a person."),[this]{Feedback.Empty();Open(TEXT("p01"));}));
 }
 else if (Id==TEXT("n01"))
 {
  Title=HasPrintout()?TEXT("N01 / COMPARE THE RECORDS"):TEXT("SARO / SHIFT FOLDER");
  R.Add(S47Kit::Button(TEXT("READ DALE'S ORIGINAL SHIFT LOG"),[this]{M->Story->Use(TEXT("anomaly:shiftlog"));}));
  if (!Data.N01Routine)
  {
   R.Add(S47Kit::Text(SceneText(TEXT("N01_ROUTINE"))));
   R.Add(S47Kit::Button(TEXT("READ AND FILE THE DATED SERVICE COPY AND SCHEDULE"),[this]{Routine();},true));
  }
  else
  {
   const auto* Service=M->Story->Docs().FindByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_SERVICE_LOG");});
   const auto* Schedule=M->Story->Docs().FindByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_RADIO_SCHEDULE");});
   const auto* Print=M->Story->Docs().FindByPredicate([](const FS47Doc& D){return D.Id==TEXT("printout");});
   FString Earlier=Service?Service->Stamp+TEXT("\n\n")+Service->Page:TEXT("SERVICE COPY MISSING");
   Earlier+=TEXT("\n\n")+(Schedule?Schedule->Stamp+TEXT("\n\n")+Schedule->Page:TEXT("SCHEDULE MISSING"));
   R.Add(S47Kit::Two(PaperPane(TEXT("EARLIER / PRESERVED SUNDAY RECORDS"),Earlier),PaperPane(Print?TEXT("LATER / ACTUAL RESIDUAL PRINTOUT"):TEXT("CURRENT SHIFT / ROUTINE CONTROL"),Print?Print->Stamp+TEXT("\n\n")+Print->Page:TEXT("DATED COPIES FILED\n\nCheck the service complaint, inspection status and scheduled programmes. Retain the original copies with the shift records."))));
   if (Print) R.Add(S47Kit::Question(TEXT("Compare the dates and what was actually measured. Do these sources establish a common cause?")));
   if (Print)
   {
    Button(TEXT("The earlier complaint does not establish the cause of this printout."),TEXT("earlier"));
    Button(TEXT("The earlier complaint proves the supply caused the residual."),TEXT("cause"));
    Button(TEXT("Insufficient evidence to connect them."),TEXT("insufficient"));
    if (Data.N01Assessment.StartsWith(TEXT("correction needed"))) Button(TEXT("Revise: record the comparison as unresolved."),TEXT("revise"));
   }
   if (Data.N01Compared) R.Add(S47Kit::Small(TEXT("Filed: unresolved cause. Follow up at the physical shift folder by requesting the actual inspection result.")));
  }
 }
 else if (Id==TEXT("reference"))
 {
  Title=TEXT("CALICHE / REGISTER A REFERENCE");
  R.Add(S47Kit::Text(TEXT("The cardboard figure is normally displayed in the souvenir-shop window. Photograph it from the marked pavement position. Keep the window, yellow price sticker and split lower edge in the frame.")));
  if (!Data.ReferencePhoto.IsEmpty()) R.Add(Picture(Data.ReferencePhoto,TEXT("YOUR ACTUAL REFERENCE EXPOSURE")));
  const FS47Doc D=ReferenceDoc();
  if (auto* T=S47Image::FromDataUrl(D.Image))
   R.Add(S47Kit::Two(S47Kit::Col({S47Kit::Sub(D.Stamp),S47Kit::Image(T,410)}),S47Kit::Paper(D.Page)));
  else R.Add(S47Kit::Paper(D.Page+TEXT("\n\nARCHIVED PRINT UNAVAILABLE / the written inventory remains available, but cannot substitute for a photographic identity check.")));
  R.Add(S47Kit::Button(TEXT("KEEP THIS DATED ARCHIVE REFERENCE"),[this]{Data.bDatedReference=true;Changed();Feedback=TEXT("Dated archive reference attached. No personal photograph or earlier visit has been invented.");Open(TEXT("reference"));},true));
 }
 else if (Id==TEXT("n02"))
 {
  Title=TEXT("N02 / COMPARE POSITION");
  R.Add(S47Kit::Two(Picture(Data.N02Photo,TEXT("FIRST POSITION / PALE OUTLINE")),Picture(Data.ControlPhoto,TEXT("SIDE POSITION / LAMP CONTROL"))));
  R.Add(S47Kit::Text(TEXT("Photograph the pale outline at the laundromat. Move to the marked side position and inspect the worklamp before taking the second view. Compare the bright edge with the lamp and the object behind it.")));
  Button(TEXT("The outline is the upright board and ordinary lamp. No independent person established."),TEXT("reflection"));
  Button(TEXT("This proves someone was standing beside the worklamp."),TEXT("person"));
  Button(TEXT("Insufficient evidence. Keep the two views for another control."),TEXT("insufficient"));
  if (Data.N02Compared) R.Add(S47Kit::Small(TEXT("Current assessment: ")+Data.N02Assessment+TEXT(". You can revise it without retaking the attached frames.")));
 }
 else if (Id==TEXT("a02"))
 {
  Title=TEXT("A02 / COMPARE IDENTITY AND POSITION");
  if (Data.bLegacyCutout) R.Add(S47Kit::Paper(TEXT("RESTORED HISTORY / the original case already recorded the cutout event. It will not replay as a new investigation. Missing evidence can be filed as insufficient; the service records and lamp control remain available.")));
  S47Kit::FW Earlier=Picture(Data.ReferencePhoto,TEXT("EARLIER / YOUR DISPLAY PHOTOGRAPH"));
  if (Data.ReferencePhoto.IsEmpty() && Data.bDatedReference)
  {
   const auto D=ReferenceDoc(); TArray<S47Kit::FW> A={S47Kit::Sub(TEXT("EARLIER / ARCHIVE PRINT, NOT YOUR EXPOSURE")),S47Kit::Small(D.Stamp)};
   if (auto* T=S47Image::FromDataUrl(D.Image)) A.Add(S47Kit::Image(T,410)); else A.Add(S47Kit::Paper(TEXT("ARCHIVE IMAGE UNAVAILABLE / use insufficient evidence until a real reference can be checked.")));
   if (bArchiveUnavailable) A.Add(S47Kit::Paper(TEXT("ARCHIVE PRINT UNAVAILABLE / this replacement card cannot establish identity. Use insufficient evidence.")));
   A.Add(S47Kit::Small(D.Page)); Earlier=S47Kit::Col(A);
  }
  R.Add(S47Kit::Two(Earlier,Picture(Data.ChangedPhoto,TEXT("LATER / YOUR PHOTOGRAPH OF THE FIGURE"))));
  R.Add(S47Kit::Question(TEXT("Compare the yellow $3.50 sticker, the split lower left edge and the position against the shop window. What do the records support?")));
  Button(TEXT("Same object, different position. Who moved it is unconfirmed."),TEXT("moved"));
  Button(TEXT("This proves the cardboard figure is alive."),TEXT("living"));
  Button(TEXT("Insufficient evidence. File only what these sources establish."),TEXT("insufficient"));
  if (Data.ReferencePhoto.IsEmpty()) R.Add(S47Kit::Button(TEXT("OPEN THE DATED HANDOVER REFERENCE"),[this]{Feedback.Empty();Open(TEXT("reference"));}));
  if (Data.A02Compared && !Data.bLegacyCutout) R.Add(S47Kit::Small(TEXT("Current assessment: ")+Data.A02Assessment+TEXT(". Next: inspect the actual sticker and split edge on the figure. A later sound is a separate observation.")));
 }
 else if (Id==TEXT("p01"))
 {
  Title=TEXT("P01 / CHECK THE APPARENT SOURCE");
  if (!Data.P01Heard) R.Add(S47Kit::Text(TEXT("No printer sound has been recorded here. Finish a comparison and return to the ordinary street. A note will be made only if the sound is actually heard.")));
  else
  {
   R.Add(S47Kit::Paper(TEXT("OBSERVED / ")+Stamp(Data.P01Clock)+TEXT("\n\nA short, familiar printer rhythm from the direction of the closed studio. Then the ordinary carrier. No new conversation.")));
   if (const auto* D=Script.FindDocument(TEXT("EV_RADIO_SCHEDULE"))) R.Add(S47Kit::Paper(FString::Join(D->Body,TEXT("\n\n"))));
   R.Add(S47Kit::Text(Data.P01Followed?TEXT("Facade inspected. No printer or person visible. The source remains unresolved."):TEXT("Approach the studio window and inspect the facade. Earl's shift ended at 04:00; his absence is ordinary. The rhythm does not establish who or what made it.")));
  }
 }
 else { Feedback.Empty(); Open(); return; }
 if (!Id.IsEmpty()) R.Add(S47Kit::Button(TEXT("OTHER FIELD CHECKS"),[this]{Feedback.Empty();Open();}));
 M->UI->Open(S47Kit::Shell(M->UI,Title,S47Kit::Col(R),1040));
}

bool FS47Anomaly::SelfTest(TArray<FString>& Lines)
{
 bool Ok=true;
 auto Check=[&](const TCHAR* Name,bool Pass){Lines.Add(FString(Pass?TEXT("ok  anomaly "):TEXT("FAIL anomaly "))+Name);Ok&=Pass;};
 FS47Anomaly A(nullptr); A.Begin();
 Check(TEXT("new case has no invented exposures or verdicts"),A.Data.ReferencePhoto.IsEmpty() && A.Data.ChangedPhoto.IsEmpty() && !A.Data.N01Routine && !A.Data.A02Compared);
 A.Data.bEarlyCamera=true; A.Data.N01Routine=true; A.Data.RoutineClock=-21500.5;
 A.Data.ReferencePhoto=TEXT("real-18"); A.Data.ReferenceClock=-19000.75;
 A.Data.ChangedPhoto=TEXT("real-04"); A.Data.ChangedClock=15000.25;
 A.Data.N02Photo=TEXT("lamp-first");A.Data.ControlPhoto=TEXT("lamp-side");A.Data.N02Control=true;
 A.Data.A02Moved=true; A.Data.P01Heard=true; A.Data.P01Clock=15123.4;
 A.Data.N01Assessment=TEXT("unresolved");
 auto Saved=S47J::Read(S47J::Write(A.Save())); FS47Anomaly B(nullptr);B.Begin(Saved);
 Check(TEXT("save v3 extension survives a JSON roundtrip"),B.Data.bEarlyCamera && B.Data.A02Moved && B.Data.P01Heard && B.Data.N02Control);
 Check(TEXT("reference IDs and actual clocks are unchanged"),B.Data.ReferencePhoto==TEXT("real-18") && B.Data.ReferenceClock==-19000.75 && B.Data.ChangedClock==15000.25);
 Check(TEXT("control and aftermath persisted"),B.Data.N02Photo==TEXT("lamp-first") && B.Data.ControlPhoto==TEXT("lamp-side") && B.Data.P01Clock==15123.4);
 // Syntetisk streng prøver serialisering, ikke avbildning eller bildekvalitet.
 A.ArchiveUrl=TEXT("synthetic-archive-photo-persistence-standin");
 auto ArchiveSaved=S47J::Read(S47J::Write(A.Save()));
 FS47Anomaly ArchiveRestored(nullptr); ArchiveRestored.Begin(ArchiveSaved);
 const auto Archive=S47J::Obj(ArchiveRestored.Save(),TEXT("archive"));
 Check(TEXT("archive identity, exact content and original clock survive JSON"),S47J::Str(Archive,TEXT("id"))==TEXT("anomaly-archive-display-1710") && S47J::Str(Archive,TEXT("url"))==A.ArchiveUrl && S47J::Num(Archive,TEXT("clock"))==-24600.);
 A.bArchiveUnavailable=true; FS47Anomaly MissingArchive(nullptr); MissingArchive.Begin(S47J::Read(S47J::Write(A.Save())));
 Check(TEXT("missing archive marker survives and blocks photographic claims"),MissingArchive.bArchiveUnavailable && S47J::Bool(S47J::Obj(MissingArchive.Save(),TEXT("archive")),TEXT("unavailable")) && !MissingArchive.Assess(TEXT("a02"),TEXT("moved")));
 Check(TEXT("observation never silently becomes conclusion"),!B.Data.A02Compared && !B.Data.N02Compared);
 Check(TEXT("living claim cannot be filed as verified"),!B.Assess(TEXT("a02"),TEXT("living")) && !B.Data.A02Compared);
 Check(TEXT("insufficient evidence is a valid revisable result"),B.Assess(TEXT("a02"),TEXT("insufficient")) && B.Data.A02Compared && B.Data.A02Assessment==TEXT("insufficient evidence"));
 Check(TEXT("missing photographic control cannot establish identity"),!B.Assess(TEXT("a02"),TEXT("moved")) && B.Data.A02Assessment==TEXT("insufficient evidence"));
 Check(TEXT("reflection needs actual stored frames"),!B.Assess(TEXT("n02"),TEXT("reflection")) && !B.Data.N02Compared);
 Check(TEXT("missing printout cannot compare a future event"),!B.Assess(TEXT("n01"),TEXT("earlier")) && !B.Data.N01Compared);
 FS47Anomaly Legacy(nullptr); Legacy.Begin(S47J::New());
 Check(TEXT("migration never promotes TownSeen or fabricates photos"),!Legacy.Data.A02Compared && !Legacy.Data.P01Heard && Legacy.Data.ReferencePhoto.IsEmpty());
 Check(TEXT("authored routine and comparison sources available"),A.Script.FindScene(TEXT("N01_ROUTINE")) && A.Script.FindScene(TEXT("N01_COMPARE")) && A.Script.FindDocument(TEXT("EV_SERVICE_LOG")) && A.Script.FindDocument(TEXT("EV_RADIO_SCHEDULE")));
 return Ok;
}

// TONIGHT-NOTE-20261109: the pause menu's TONIGHT card as the night supervisor's note (S47TonightNote.h).
#include "S47TonightNote.h"
#include "S47HudGauges.h"
#include "S47Script.h"

namespace S47TonightNoteLocal
{
 // (using-declarations, not a using-directive: a unity build's other files have their own FLine and Key at file scope)
 using S47TonightNote::EMark; using S47TonightNote::FLine; using S47TonightNote::FNote; using S47TonightNote::FIn; using S47TonightNote::FItem;
 using S47TonightNote::FWindow; using S47TonightNote::MostLines; using S47TonightNote::MostDone; using S47TonightNote::MostChars;
 using S47TonightNote::HourMinute; using S47TonightNote::LineText;
 struct FWork
 {
  FLine L;
  double Order = 0;            // where it stands in the task list
  double Start = -1, End = -1; // its window, if it has one
  FString Key;                 // its card, to match the objective and the windows by
  bool Windowed() const { return Start >= 0; }
 };
 constexpr double Day = 86400.;
 // Whose night a world clock falls in, as FS47FreeformSession::NightOf counts it (from 08:00 to 08:00).
 int32 NightOf(double At) { return FMath::Max(1, int32(FMath::FloorToDouble((At - 8 * 3600.) / Day)) + 1); }
 FString Key(const FString& Card)
 {
  FString K = Card.TrimStartAndEnd().ToLower();
  while (K.ReplaceInline(TEXT("  "), TEXT(" ")) > 0) {}
  while (K.EndsWith(TEXT(".")) || K.EndsWith(TEXT(" "))) K.LeftChopInline(1);
  return K;
 }
 // FS47FreeformSession::JobLine: "D02 / FF-D0002-0001 / CYG A / FILED"
 bool IsJob(const FString& T)
 {
  return T.Len() > 6 && T[0] == TEXT('D') && FChar::IsDigit(T[1]) && FChar::IsDigit(T[2]) && T.Mid(3, 3) == TEXT(" / ");
 }
 // The post as the list says it: FS47Mail::TaskLine, and FS47Predecessor::TaskLine's letters (FS47FreeformSession::LetterWaits).
 bool IsPost(const FString& T)
 {
  return T.StartsWith(TEXT("MAILBOX / ")) || T.StartsWith(TEXT("MAIL ORDER / ")) || T.StartsWith(TEXT("OPS / There is a letter")) || T.StartsWith(TEXT("OPS / Dr. Ward's assessment"));
 }
 // What the station is (FS47Predecessor::TaskLine once nothing waits): not a thing to do tonight.
 // TASKS-TEN-20261113 (change 3): and every line that says how things stand rather than what to do: the post's flag up for the carrier,
 // a parcel due, the tired state, the card count. (A file's logged hours, HABITS, is no task either: it is an open matter, Unknowns.)
 bool IsStatus(const FString& T)
 {
  return T.StartsWith(TEXT("SARO under review")) || T.StartsWith(TEXT("SARO KEPT OPEN")) || T.StartsWith(TEXT("SARO CLOSES")) || T.StartsWith(TEXT("MAIL ORDER /"))
   || (T.StartsWith(TEXT("MAILBOX / ")) && T.EndsWith(TEXT("the flag up for the carrier."))) || T.StartsWith(TEXT("YOU / ")) || T.StartsWith(TEXT("Job cards:"));
 }
 // TASKS-TEN-20261113 (change 9): the action in one sentence and one step: what comes after a ", then" is the next step's, not this one's
 FString OneStep(const FString& What)
 {
  FString Out = What;
  for (const TCHAR* Then : { TEXT(", then "), TEXT("; then "), TEXT(": then "), TEXT(" then: ") })
  {
   const int32 At = Out.Find(Then, ESearchCase::IgnoreCase);
   if (At > 8) { Out.LeftInline(At); Out.TrimEndInline(); if (!Out.EndsWith(TEXT("."))) Out += TEXT("."); }
  }
  return Out;
 }
 // The room as its sign has it (S47SaroDoorDetail, S47RoomLights): RECORDS, CORRIDOR, OPERATIONS.
 FString Sign(const FString& Head)
 {
  const FString Up = Head.TrimStartAndEnd().ToUpper();
  static const TCHAR* Signs[][2] = {
   { TEXT("RECORDS ROOM"), TEXT("RECORDS") }, { TEXT("SARO RECORDS"), TEXT("RECORDS") }, { TEXT("SARO OPERATIONS"), TEXT("OPERATIONS") },
   { TEXT("SOUTH CORRIDOR"), TEXT("CORRIDOR") }, { TEXT("CONTROL ROOM DESK"), TEXT("CONTROL ROOM") },
  };
  for (const auto& S : Signs) if (Up == S[0]) return S[1];
  return Up;
 }
 // A place a task in words names (Story's lists: "Find the reference record (south corridor)"), as its sign has it.
 FString Named(const FString& Text)
 {
  const FString Low = Text.ToLower();
  static const TCHAR* Places[][2] = {
   { TEXT("photo lab"), TEXT("PHOTO LAB") }, { TEXT("darkroom"), TEXT("PHOTO LAB") }, { TEXT("wet bench"), TEXT("PHOTO LAB") },
   { TEXT("control room"), TEXT("CONTROL ROOM") }, { TEXT("south corridor"), TEXT("CORRIDOR") }, { TEXT("records room"), TEXT("RECORDS") },
   { TEXT("operations"), TEXT("OPERATIONS") }, { TEXT("service yard"), TEXT("SERVICE YARD") }, { TEXT("truck pad"), TEXT("TRUCK PAD") },
   { TEXT("station 01"), TEXT("STATION 01") }, { TEXT("mesa diner"), TEXT("MESA DINER") }, { TEXT("box at the road"), TEXT("MAILBOX") },
   { TEXT("mailbox"), TEXT("MAILBOX") },
  };
  for (const auto& P : Places) if (Low.Contains(P[0])) return P[1];
  return FString();
 }
 // The first sentence whole, and the next ones while they fit Most (a doctor's "Dr." not counted as an end).
 FString Sentences(const FString& Body, int32 Most)
 {
  FString Out, Piece;
  for (int32 I = 0; I < Body.Len(); ++I)
  {
   const TCHAR Ch = Body[I];
   Piece.AppendChar(Ch);
   const bool bLast = I + 1 == Body.Len();
   if (!bLast && !((Ch == TEXT('.') || Ch == TEXT(';') || Ch == TEXT('!') || Ch == TEXT('?')) && Body[I + 1] == TEXT(' '))) continue;
   const FString Low = Piece.ToLower();
   if (!bLast && Ch == TEXT('.') && (Low.EndsWith(TEXT(" dr.")) || Low == TEXT("dr.") || Low.EndsWith(TEXT(" mr.")) || Low.EndsWith(TEXT(" st.")))) continue;
   FString S = Piece.TrimStartAndEnd();
   Piece.Reset();
   if (S.EndsWith(TEXT(";"))) { S.LeftChopInline(1); S += TEXT("."); }
   if (S.IsEmpty()) continue;
   if (Out.IsEmpty()) Out = S;
   else if (Out.Len() + 1 + S.Len() <= Most) Out += TEXT(" ") + S;
   else break;
  }
  return Out;
 }
 // A task in the card's words (S47HudGauges::CardLine: the screens' short names, the place before its //), what to do after the
 // place in its first sentences, written as a sentence.
 // D45-20261110: NOW (bAll) has room for the second sentence the note leaves off ("Drive out and look before noon.")
 void Split(const FString& Text, FString& Where, FString& What, int32 Most = MostChars)
 {
  const FString Card = S47HudGauges::CardLine(Text, 1000);
  FString Head, Body;
  if (Card.Split(TEXT(" // "), &Head, &Body)) { Where = Sign(Head); What = Sentences(Body, Most); }
  else What = Sentences(Card, Most);
  What = S47HudGauges::Handwritten(What.TrimStartAndEnd(), false);
  What.ReplaceInline(TEXT("ff-D"), TEXT("FF-D"), ESearchCase::CaseSensitive); // a job's id ("FF-D0002-0001") keeps its capitals
  if (Where.IsEmpty()) Where = Named(Text);
 }
 FString Window(double Start, double End, double Clock, bool bFreeform)
 {
  FString W = HourMinute(Start) + TEXT("-") + HourMinute(End);
  if (bFreeform && Clock >= 0 && NightOf(Start) != NightOf(Clock)) W += FString::Printf(TEXT(" (day %d)"), NightOf(Start));
  return W;
 }
 bool Clean(const FIn& In, const FString& Text)
 {
  if (!In.bFreeform) return true;
  const FString Why = S47Script::LintText(Text);
  if (Why.IsEmpty()) return true;
  UE_LOG(LogTemp, Display, TEXT("S47: tonight's note leaves a line off (%s): %s"), *Why, *Text);
  return false;
 }
}

FString S47TonightNote::HourMinute(double Clock)
{
 if (!FMath::IsFinite(Clock) || Clock < 0) return TEXT("--:--");
 const int32 Minutes = FMath::FloorToInt(FMath::Fmod(Clock, 86400.) / 60.);
 return FString::Printf(TEXT("%02d:%02d"), (Minutes / 60) % 24, Minutes % 60);
}

FString S47TonightNote::LineText(const FLine& Line)
{
 // (TASKS-TEN-20261113: a line with a giver says its window in its tag, TagText)
 FString Out = Line.When.IsEmpty() || !Line.Giver.IsEmpty() ? FString() : Line.When + TEXT("  ");
 if (!Line.Where.IsEmpty()) Out += Line.Where + TEXT(": ");
 return Out + Line.What;
}

FString S47TonightNote::TagText(const FLine& Line)
{
 if (Line.Giver.IsEmpty()) return FString();
 // THE LAND never says what a thing is worth (no spoiler)
 return Line.Giver + TEXT(" / ") + (Line.Span.IsEmpty() ? FString(TEXT("tonight")) : Line.Span) + TEXT(" / ") + (Line.Giver == TEXT("THE LAND") || Line.Worth.IsEmpty() ? FString(TEXT("unknown")) : Line.Worth);
}

const TCHAR* S47TonightNote::KindHead(uint8 Kind)
{
 return Kind == 0 ? TEXT("WORK") : Kind == 1 ? TEXT("FOLLOW-UP") : TEXT("YOUR OWN TIME");
}

FString S47TonightNote::Plain(const FNote& Note)
{
 TArray<FString> Out;
 for (const FLine& L : Note.Lines)
 {
  const FString Tag = TagText(L);
  Out.Add((L.Mark == EMark::Done ? TEXT("[x] ") : L.Mark == EMark::Next ? TEXT("[>] ") : TEXT("[ ] ")) + (L.bPinned ? TEXT("* ") : TEXT("")) + LineText(L) + (Tag.IsEmpty() ? FString() : TEXT("  (") + Tag + TEXT(")")));
 }
 // TASKS-TEN-20261113 (change 6): Freeform's count is the open matters (and tonight's rows with no room), which NOW has in full
 if (Note.More > 0) Out.Add(Note.Matters.Num() || Note.Lines.ContainsByPredicate([](const FLine& L) { return !L.Giver.IsEmpty(); }) ? FString::Printf(TEXT("+%d open matters in the notebook"), Note.More) : FString::Printf(TEXT("+%d more in the notebook"), Note.More));
 for (const FLine& L : Note.Matters) Out.Add(TEXT("~ ") + L.Matter.ToUpper() + TEXT(" / ") + LineText(L) + TEXT("  (") + TagText(L) + TEXT(")"));
 for (const FString& A : Note.Also) Out.Add(A);
 if (!Note.Clock.IsEmpty()) Out.Add(Note.Clock);
 return FString::Join(Out, TEXT("\n"));
}

namespace S47TonightNoteLocal
{
 // TASKS-TEN-20261113 (changes 6 and 9): what a row is, from the task's own words (keep these heads when a line is reworded): tonight's
 // WORK (cards, case windows, result sheets: Kind 0), FOLLOW-UP (the bearing, worth a look, reported lights, the dossier's extra
 // bearing, what Dale asked: 1) or YOUR OWN TIME (the camera, a repair with the parts, the round, the post: 2); or an open matter
 // (Cases, Station, Post, Unknowns, Ward) that stays until it is closed. Who gives it and what it is worth, in the fixed short form.
 struct FClass { uint8 Kind = 0; FString Matter, Giver, Span, Worth; };
 bool Ward(const FString& Up) { return Up.Contains(TEXT("DR. WARD")) || Up.Contains(TEXT("COUNTY INSPECTOR")); }
 FString Dollars(const FString& Up)
 {
  int32 At = INDEX_NONE;
  if (!Up.FindChar(TEXT('$'), At)) return FString();
  int32 End = At + 1; while (End < Up.Len() && FChar::IsDigit(Up[End])) ++End;
  return End > At + 1 ? Up.Mid(At, End - At) : FString();
 }
 FClass Classify(const FString& Source)
 {
  FClass C; C.Giver = TEXT("STATION"); C.Span = TEXT("tonight"); C.Worth = TEXT("no pay");
  const FString Up = Source.TrimStartAndEnd().ToUpper();
  const auto Land = [&C](uint8 Kind, const TCHAR* Span) { C.Kind = Kind; C.Giver = TEXT("THE LAND"); C.Span = Span; C.Worth = TEXT("unknown"); };
  // open matters
  if (Up.StartsWith(TEXT("UNKNOWN ")) && Up.Contains(TEXT(" / LOGGED SO FAR AT "))) { Land(1, TEXT("open")); C.Matter = TEXT("Unknowns"); return C; }
  if (Up.StartsWith(TEXT("INSTRUMENT BOX /"))) { Land(2, TEXT("open")); C.Matter = TEXT("Post"); return C; }
  if (IsPost(Source))
  {
   C.Kind = 2; C.Matter = TEXT("Post");
   if (Ward(Up)) { C.Giver = TEXT("WARD"); C.Span = Up.Contains(TEXT("MONDAY")) ? TEXT("by Monday") : TEXT("open"); C.Worth = TEXT("counts for the review"); }
   else Land(2, TEXT("open"));
   return C;
  }
  if (Up.StartsWith(TEXT("CASE /")) && !Up.Contains(TEXT(": RUN JOB ")) && !Up.Contains(TEXT("TEAR OFF THE REPLY")))
  { C.Matter = TEXT("Cases"); C.Span = TEXT("open"); C.Worth = TEXT("counts for the case"); return C; }
  // FOLLOW-UP: the land's
  if (Up.StartsWith(TEXT("BEARING SLIP /")) || Up.StartsWith(TEXT("REPORTED:")) || Up.StartsWith(TEXT("WORTH A LOOK /"))) { Land(1, TEXT("until noon")); return C; }
  if (Up.StartsWith(TEXT("DALE ASKED /")) || Up.StartsWith(TEXT("ARRAY / THE STATION KNOWS WHAT TO LISTEN FOR"))) { Land(1, TEXT("until 08:00")); return C; }
  if (Up.StartsWith(TEXT("YARD / LIGHTS"))) { Land(1, TEXT("around 21:00")); return C; }
  // YOUR OWN TIME
  if (Up.StartsWith(TEXT("ROUND /"))) { C.Kind = 2; C.Span = TEXT("until 19:00"); const FString D = Dollars(Up); C.Worth = D.IsEmpty() ? FString(TEXT("on the sheet")) : D; return C; }
  if (Up.StartsWith(TEXT("PHOTO LAB"))) { C.Kind = 2; C.Span = TEXT("until 08:00"); C.Worth = TEXT("allowance"); return C; }
  if (Up.StartsWith(TEXT("LEFT BEHIND /"))) { C.Kind = 2; C.Span = TEXT("open"); C.Worth = TEXT("your kit"); return C; }
  if (Up.StartsWith(TEXT("STATION GOAL /")) || Up.StartsWith(TEXT("ANTENNA ")))
  {
   C.Kind = 2; C.Span = TEXT("by daylight");
   int32 Plus = Up.Find(TEXT("(+"));
   C.Worth = Plus != INDEX_NONE ? Up.Mid(Plus + 1, Up.Find(TEXT(")"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Plus) - Plus - 1).ToLower() : FString(TEXT("the array"));
   return C;
  }
  if (Up.Contains(TEXT("PAID WORK IS DONE")) || Up.Contains(TEXT("PAID WORK DONE")) || Up.Contains(TEXT("GO OUT WITH THE CAMERA"))) { Land(2, TEXT("until 08:00")); return C; }
  // WORK: the station's, with a clock (blocking now first)
  if (Up.StartsWith(TEXT("PRINTER")) || Up.StartsWith(TEXT("TELEX")) || Up.StartsWith(TEXT("FEEDER /")) || Up.StartsWith(TEXT("SERVICE TRUCK /")) || Up.StartsWith(TEXT("TRUCK /"))
   || Up.Contains(TEXT("/ DIAGNOSE AT THE STATUS PC")) || Up.StartsWith(TEXT("SERVICE BAY")) || Up.StartsWith(TEXT("NO POWER")))
  { C.Span = TEXT("now"); return C; }
  if (Up.StartsWith(TEXT("CASE /")) || Up.StartsWith(TEXT("FIELD TEST /")) || Up.StartsWith(TEXT("CONTROL ROOM: RETURN")) || Up.StartsWith(TEXT("FIELD POST")) || Up.StartsWith(TEXT("SARO NORTH FACADE"))
   || Up.StartsWith(TEXT("ARRAY SERVICE GATE")) || Up.StartsWith(TEXT("SEALED SERVICE GATE")) || Up.StartsWith(TEXT("SERVICE GATE")) || Up.StartsWith(TEXT("GATE CONTROL")) || Up.StartsWith(TEXT("CASE BOARD")))
  { C.Span = TEXT("until 08:00"); C.Worth = TEXT("counts for the case"); return C; }
  return C;
 }
 FNote BuildNote(const FIn& In, bool bAll)
{
 FNote N;
 TArray<FWork> All;
 TArray<FLine> Matters;
 const FString NextKey = Key(S47HudGauges::CardLine(In.Next));
 auto IsNextKey = [&NextKey](const FString& K) { return !NextKey.IsEmpty() && (K == NextKey || (K.Len() >= 12 && NextKey.StartsWith(K))); };
 // D45-20261110: NOW (bAll) had room for the second sentence; TASKS-TEN-20261113 (change 9): in Freeform one sentence on every surface
 // (and one step: OneStep), the task's whole words stay in the line's Source (NOW's WHY)
 const int32 Most = In.bFreeform ? 1 : MostChars;
 auto AddMatter = [&](const FString& Text, const FClass& C)
 {
  FLine L; L.Source = Text; L.Matter = C.Matter; L.Giver = C.Giver; L.Span = C.Span; L.Worth = C.Worth; L.Kind = C.Kind;
  Split(Text, L.Where, L.What, Most); L.What = OneStep(L.What); L.Key = Key(S47HudGauges::CardLine(Text));
  if (!L.What.IsEmpty() && !Matters.ContainsByPredicate([&L](const FLine& X) { return X.Key == L.Key; }) && Clean(In, LineText(L))) Matters.Add(L);
 };
 int32 Filed = 0;
 for (int32 I = 0; I < In.Tasks.Num(); ++I)
 {
  const FString Text = In.Tasks[I].Text.TrimStartAndEnd();
  const bool bDone = In.Tasks[I].bDone;
  if (Text.IsEmpty()) continue;
  FClass C;
  if (In.bFreeform)
  {
   // the job cards are rows of their own (FIn::Cards); filed ones counted here
   if (IsJob(Text)) { Filed += bDone ? 1 : 0; continue; }
   if (IsStatus(Text)) continue;
   C = Classify(Text);
   const bool bNextRow = IsNextKey(Key(S47HudGauges::CardLine(Text))) || (!In.NextRow.IsEmpty() && Text == In.NextRow);
   // the station's one row (the array chain with the work order inside: FIn::Matters) stands for its lines; with the parts in hand by
   // day the repair is also tonight's own time
   const bool bStation = In.StationRows.Contains(Text);
   if (bStation && !bNextRow && !(Text.StartsWith(TEXT("STATION GOAL /")) && Text.EndsWith(TEXT("YOU HAVE THE PARTS.")))) continue;
   // an open matter stays in OPEN MATTERS (no ticks, until it is closed), unless the HUD's ladder made it the step tonight
   if (!C.Matter.IsEmpty() && !bStation)
   {
    if (!bNextRow) { if (!bDone) AddMatter(Text, C); continue; }
    C.Matter.Reset();
   }
   if (In.WaitingCases.ContainsByPredicate([&Text](const FString& Title) { return !Title.IsEmpty() && Text.StartsWith(TEXT("CASE / ") + Title); })) continue;
  }
  FWork W;
  W.L.Mark = bDone ? EMark::Done : EMark::Open;
  W.Order = I;
  W.Key = Key(S47HudGauges::CardLine(Text));
  W.L.Source = Text;
  W.L.Kind = C.Kind; W.L.Giver = C.Giver; W.L.Span = C.Span; W.L.Worth = C.Worth;
  Split(Text, W.L.Where, W.L.What, Most);
  if (In.bFreeform) W.L.What = OneStep(W.L.What);
  for (const FWindow& Wd : In.Windows) if (Wd.Text.TrimStartAndEnd() == Text) { W.Start = Wd.Start; W.End = Wd.End; }
  // the same thing twice (a case's step in the list and its window, the objective's card) is one line
  if (W.L.What.IsEmpty() || All.ContainsByPredicate([&W](const FWork& X) { return X.Key == W.Key; })) continue;
  All.Add(W);
 }
 // a window with no line of its own in the list
 for (const FWindow& Wd : In.Windows)
 {
  const FString K = Key(S47HudGauges::CardLine(Wd.Text));
  if (Wd.Text.TrimStartAndEnd().IsEmpty() || All.ContainsByPredicate([&K](const FWork& X) { return X.Key == K; })) continue;
  FWork W; W.Order = In.Tasks.Num() + 1; W.Key = K; W.Start = Wd.Start; W.End = Wd.End; W.L.Source = Wd.Text.TrimStartAndEnd();
  if (In.bFreeform) { const FClass C = Classify(W.L.Source); W.L.Kind = 0; W.L.Giver = C.Giver; W.L.Span = C.Span; W.L.Worth = C.Worth; }
  Split(Wd.Text, W.L.Where, W.L.What, Most);
  All.Add(W);
 }
 // TASKS-TEN-20261113 (change 4): the cards up now on one row, counted as the HUD counts them; each card not up yet on its own row,
 // with its window
 {
  int32 Up = 0, UpPay = 0; double UpShut = -1;
  for (const FIn::FCard& Cd : In.Cards) if (Cd.bUp) { ++Up; UpPay += Cd.Pay; if (Cd.Shut >= 0 && (UpShut < 0 || Cd.Shut < UpShut)) UpShut = Cd.Shut; }
  if (Up > 0)
  {
   FWork W; W.Order = -1.5; W.Key = TEXT("cards"); W.L.Where = TEXT("OPS"); W.L.Source = W.Key;
   W.L.What = Up == 1 ? FString(TEXT("One job card up.")) : FString::Printf(TEXT("%d job cards up."), Up);
   W.L.Giver = TEXT("STATION"); W.L.Span = UpShut >= 0 ? TEXT("until ") + HourMinute(UpShut) : FString(TEXT("until 08:00")); W.L.Worth = FString::Printf(TEXT("$%d"), UpPay);
   All.Add(W);
  }
  for (const FIn::FCard& Cd : In.Cards)
  {
   if (Cd.bUp || Cd.Up < 0) continue;
   FWork W; W.Order = In.Tasks.Num() + 2; W.Key = TEXT("card:") + Cd.Id; W.Start = Cd.Up; W.End = Cd.Shut; W.L.Where = TEXT("TRACKING PC"); W.L.Source = W.Key;
   W.L.What = (Cd.bNightWatch ? TEXT("Night watch card: ") : TEXT("Evening card: ")) + S47HudGauges::Handwritten(Cd.Source, true) + TEXT(".");
   W.L.Giver = TEXT("STATION"); W.L.Worth = FString::Printf(TEXT("$%d"), Cd.Pay);
   All.Add(W);
  }
 }
 // tonight's filed cards, ticked off on one line
 if (Filed > 0)
 {
  FWork W; W.L.Mark = EMark::Done; W.Order = -2; W.L.Where = TEXT("OPS"); W.Key = TEXT("filed cards"); W.L.Source = W.Key;
  W.L.What = Filed == 1 ? FString(TEXT("One job card registered and filed.")) : FString::Printf(TEXT("%d job cards registered and filed."), Filed);
  W.L.Giver = TEXT("STATION"); W.L.Span = TEXT("tonight"); W.L.Worth = TEXT("paid");
  All.Insert(W, 0);
 }
 for (FWork& W : All) if (W.Windowed()) { W.L.When = Window(W.Start, W.End, In.Clock, In.bFreeform); if (In.bFreeform) W.L.Span = W.L.When; }
 // D45-20261110: how much each matters (a window open now is DO FIRST)
 for (FWork& W : All) W.L.Rank = S47TonightNote::RankOf(W.L.Source, W.Windowed() && In.Clock >= W.Start && In.Clock < W.End);
 // the spoiler limits before anything is chosen, so that what is counted is what could be shown
 All.RemoveAll([&In](const FWork& W) { return !Clean(In, LineText(W.L)); });
 for (FWork& W : All) W.L.Key = W.Key;
 // TASKS-TEN-20261113 (change 6): a time-boxed matter leaves tonight's list when its window has shut
 if (In.bFreeform && In.Clock >= 0) All.RemoveAll([&In](const FWork& W) { return W.L.Mark != EMark::Done && W.Windowed() && W.End <= In.Clock; });
 // What was done tonight stays struck until 08:00, also when the list has let it go (PT-NOTEBOOK-20261109).
 if (In.bFreeform && In.Done && In.Clock >= 0)
 {
  S47TonightNote::FDone& Mem = *In.Done;
  const int32 Night = NightOf(In.Clock);
  if (Night != Mem.Night || In.Clock + 60 < Mem.Last || In.Game != Mem.Game) { Mem.Lines.Reset(); Mem.Order.Reset(); Mem.Night = Night; Mem.Game = In.Game; }
  Mem.Last = In.Clock;
  for (const FWork& W : All)
   if (W.L.Mark == EMark::Done) { if (!Mem.Lines.Contains(W.Key)) Mem.Order.Add(W.Key); FLine L = W.L; L.When.Reset(); Mem.Lines.Add(W.Key, L); }
  for (int32 I = 0; I < Mem.Order.Num(); ++I)
  {
   const FString& K = Mem.Order[I];
   if (All.ContainsByPredicate([&K](const FWork& X) { return X.Key == K; })) continue;
   const FLine* L = Mem.Lines.Find(K);
   if (!L) continue;
   FWork W; W.L = *L; W.L.Mark = EMark::Done; W.Key = K; W.Order = -100 + I;
   All.Add(W);
  }
 }

 // The next step. TASKS-TEN-20261113 (change 2, one arbiter): in Freeform the HUD's ladder has chosen it (FS47FreeformSession::
 // ObjectiveRow: blocking now, a window open now, a paid card up, the night watch, a deadline before noon, free time), and the note
 // marks that row, so that the HUD, this note, NOW's arrow and the workstations' NEXT never disagree. In Story a window open now
 // comes first; otherwise the objective's own line, or, as Story's list says it in other words, the first thing not done.
 const double Clock = In.Clock;
 auto IsOpen = [](const FWork& W) { return W.L.Mark != EMark::Done; };
 int32 Next = INDEX_NONE;
 if (In.bFreeform && !In.NextRow.IsEmpty())
 {
  const FString RowKey = In.NextRow.StartsWith(TEXT("card")) && !In.NextRow.Contains(TEXT(" ")) ? In.NextRow : Key(S47HudGauges::CardLine(In.NextRow));
  Next = All.IndexOfByPredicate([&](const FWork& W) { return IsOpen(W) && W.Key == RowKey; });
  // a card's row is said in the HUD's words, so that the two read the same
  if (Next != INDEX_NONE && All[Next].Key.StartsWith(TEXT("card")) && !In.Next.IsEmpty()) { All[Next].L.Where.Reset(); Split(In.Next, All[Next].L.Where, All[Next].L.What, Most); }
 }
 if (Next == INDEX_NONE && !In.bFreeform) Next = All.IndexOfByPredicate([&](const FWork& W) { return IsOpen(W) && W.Windowed() && Clock >= W.Start && Clock < W.End; });
 if (Next == INDEX_NONE && !NextKey.IsEmpty()) Next = All.IndexOfByPredicate([&](const FWork& W) { return IsOpen(W) && IsNextKey(W.Key); });
 if (Next == INDEX_NONE && !In.bFreeform) Next = All.IndexOfByPredicate(IsOpen);
 if (Next != INDEX_NONE && !In.bFreeform && All[Next].L.Where.IsEmpty() && !In.Next.IsEmpty())
 {
  FString Head;
  if (S47HudGauges::CardLine(In.Next).Split(TEXT(" // "), &Head, nullptr)) All[Next].L.Where = Sign(Head);
 }
 if (Next == INDEX_NONE && !NextKey.IsEmpty())
 {
  FWork W; W.Order = -1; W.Key = NextKey; W.L.Key = NextKey; W.L.Source = In.Next;
  Split(In.Next, W.L.Where, W.L.What, Most);
  if (In.bFreeform) { const FClass C = Classify(In.Next); W.L.Kind = C.Kind; W.L.Giver = C.Giver; W.L.Span = C.Span; W.L.Worth = C.Worth; W.L.What = OneStep(W.L.What); }
  if (!W.L.What.IsEmpty() && Clean(In, LineText(W.L))) { All.Add(W); Next = All.Num() - 1; }
 }
 // a window that has not opened is no next step while there is something to do before it (in Freeform the ladder knows: FieldWindowWaits)
 if (!In.bFreeform && Next != INDEX_NONE && All[Next].Windowed() && Clock < All[Next].Start)
 {
  const int32 Before = All.IndexOfByPredicate([&](const FWork& W) { return IsOpen(W) && !W.Windowed(); });
  if (Before != INDEX_NONE) Next = Before;
 }
 // TASKS-TEN-20261113 (change 10): the row the player pinned
 if (In.bFreeform && !In.Pinned.IsEmpty()) for (FWork& W : All) W.L.bPinned = IsOpen(W) && W.Key == In.Pinned;

 if (!In.bFreeform)
 {
  // Story: the list's own order; what there is room for: the next step, the windows, the rest, and the last things done
  TArray<FWork> Done, Rest, Windows;
  for (int32 I = 0; I < All.Num(); ++I)
  {
   if (I == Next) continue;
   (!IsOpen(All[I]) ? Done : All[I].Windowed() ? Windows : Rest).Add(All[I]);
  }
  Windows.Sort([](const FWork& A, const FWork& B) { return A.Start < B.Start; });
  Rest.Sort([](const FWork& A, const FWork& B) { return A.Order < B.Order; });
  Done.Sort([](const FWork& A, const FWork& B) { return A.Order < B.Order; });
  const int32 HasNext = Next != INDEX_NONE ? 1 : 0;
  const int32 OpenCount = Windows.Num() + Rest.Num();
  constexpr int32 StoryLines = 6, StoryDone = 2;
  int32 DoneShown = bAll ? Done.Num() : FMath::Min(Done.Num(), StoryDone);
  if (!bAll && HasNext + OpenCount + DoneShown > StoryLines) DoneShown = FMath::Min(Done.Num(), 1);
  int32 Room = bAll ? OpenCount : FMath::Max(0, StoryLines - HasNext - DoneShown);
  const int32 WindowsShown = FMath::Min(Windows.Num(), Room); Room -= WindowsShown;
  const int32 RestShown = FMath::Min(Rest.Num(), Room);
  N.More = OpenCount - WindowsShown - RestShown;
  for (int32 I = Done.Num() - DoneShown; I < Done.Num(); ++I) N.Lines.Add(Done[I].L);
  if (HasNext) { FLine L = All[Next].L; L.Mark = EMark::Next; N.Lines.Add(L); }
  for (int32 I = 0; I < RestShown; ++I) N.Lines.Add(Rest[I].L);
  for (int32 I = 0; I < WindowsShown; ++I) N.Lines.Add(Windows[I].L);
  return N;
 }

 // TASKS-TEN-20261113 (change 6): Freeform's note by group, WORK, FOLLOW-UP, YOUR OWN TIME; in each the next step first, then what
 // matters most (RankOf), the windows by their time, the list's order. The pause menu's card has room for MostLines (the next step
 // always among them, a thing done tonight only if there is room), and one "+N open matters" for the rest and NOW's OPEN MATTERS.
 TArray<FWork> Open, Done;
 for (int32 I = 0; I < All.Num(); ++I)
 {
  FWork W = All[I];
  if (I == Next) W.L.Mark = EMark::Next;
  (IsOpen(W) ? Open : Done).Add(W);
 }
 Open.Sort([](const FWork& A, const FWork& B)
 {
  if (A.L.Kind != B.L.Kind) return A.L.Kind < B.L.Kind;
  if ((A.L.Mark == EMark::Next) != (B.L.Mark == EMark::Next)) return A.L.Mark == EMark::Next;
  if (A.L.Rank != B.L.Rank) return A.L.Rank < B.L.Rank;
  if (A.Windowed() != B.Windowed()) return A.Windowed();
  return A.Windowed() ? A.Start < B.Start : A.Order < B.Order;
 });
 Done.Sort([](const FWork& A, const FWork& B) { return A.Order < B.Order; });
 TArray<FWork> Shown = Open;
 if (!bAll && Shown.Num() > MostLines)
 {
  // the next step and the pinned row keep their place; the rest in order while there is room
  TArray<FWork> Keep;
  for (const FWork& W : Shown) if (W.L.Mark == EMark::Next || W.L.bPinned) Keep.Add(W);
  for (const FWork& W : Shown) if (Keep.Num() < MostLines && W.L.Mark != EMark::Next && !W.L.bPinned) Keep.Add(W);
  Keep.Sort([&Open](const FWork& A, const FWork& B) { return Open.IndexOfByPredicate([&A](const FWork& X) { return X.Key == A.Key; }) < Open.IndexOfByPredicate([&B](const FWork& X) { return X.Key == B.Key; }); });
  Shown = Keep;
 }
 const int32 DoneShown = bAll ? Done.Num() : FMath::Min(Done.Num(), FMath::Max(0, FMath::Min(MostDone, MostLines - Shown.Num())));
 for (const FWork& W : Shown) N.Lines.Add(W.L);
 for (int32 I = Done.Num() - DoneShown; I < Done.Num(); ++I) N.Lines.Add(Done[I].L);
 // OPEN MATTERS: the caller's (the station's one row, Ward's), the list's, the cases waiting on evidence; by group
 for (const FLine& L : In.Matters) if (!L.What.IsEmpty() && !Matters.ContainsByPredicate([&L](const FLine& X) { return X.Key == L.Key; }) && Clean(In, LineText(L))) Matters.Add(L);
 for (const FString& T : In.WaitingCases)
 {
  if (Matters.ContainsByPredicate([&T](const FLine& X) { return X.Source.StartsWith(TEXT("CASE / ") + T); })) continue;
  FLine L; L.Matter = TEXT("Cases"); L.Where = TEXT("CASE BOARD"); L.What = S47HudGauges::Handwritten(T, true) + TEXT(": waiting on evidence."); L.Key = TEXT("case-waiting:") + Key(T);
  L.Giver = TEXT("STATION"); L.Span = TEXT("open"); L.Worth = TEXT("counts for the case"); L.Source = TEXT("CASE / ") + T + TEXT(": waiting on evidence; choose a test on the OPS case board");
  if (Clean(In, LineText(L))) Matters.Add(L);
 }
 static const TCHAR* const MatterOrder[] = { TEXT("Cases"), TEXT("Station"), TEXT("Post"), TEXT("Unknowns"), TEXT("Ward") };
 const auto MatterRank = [](const FString& G) { for (int32 I = 0; I < 5; ++I) if (G == MatterOrder[I]) return I; return 5; };
 Matters.StableSort([&MatterRank](const FLine& A, const FLine& B) { return MatterRank(A.Matter) < MatterRank(B.Matter); });
 for (FLine& L : Matters) L.bPinned = !In.Pinned.IsEmpty() && L.Key == In.Pinned;
 N.Matters = Matters;
 N.More = bAll ? 0 : Open.Num() - Shown.Num() + Matters.Num();

 // What the clock allows now.
 if (Clock >= 0)
 {
  // (a card not up yet is the evening's line below, not a case window: TASKS-TEN-20261113)
  const auto CaseWindow = [](const FWork& W) { return W.Windowed() && !W.Key.StartsWith(TEXT("card:")); };
  const FWork* Now = All.FindByPredicate([&](const FWork& W) { return IsOpen(W) && CaseWindow(W) && Clock >= W.Start && Clock < W.End; });
  const FWork* Soon = nullptr;
  for (const FWork& W : All) if (IsOpen(W) && CaseWindow(W) && W.Start > Clock && (!Soon || W.Start < Soon->Start)) Soon = &W;
  const double Hour = FMath::Fmod(Clock, Day) / 3600.;
  FString Say;
  // LEAD-FIX-20261110: as the gates in FS47CaseFieldTests have it: the roof wants a minute heard in the control room; the gate and the
  // field post count 20 minutes skipped (FS47FreeformCases::UnwatchedAllowance); a job's window simply has to be worked in.
  if (Now) Say = TEXT("The window is open until ") + HourMinute(Now->End) + (Now->L.Source.StartsWith(TEXT("CONTROL ROOM")) ? TEXT(". One minute of it heard in the control room will do, in wind like the ribbon's; skipped time is not heard.")
   : Now->L.Source.Contains(TEXT("SERVICE GATE")) || Now->L.Source.Contains(TEXT("FIELD POST")) ? TEXT(". Up to 20 minutes of it may be skipped or slept; more, and it records nothing.") : TEXT(". Time skipped past it records nothing."));
  else if (Soon)
  {
   const FString At = NightOf(Soon->Start) != NightOf(Clock) ? FString::Printf(TEXT("%s on shift day %d"), *HourMinute(Soon->Start), NightOf(Soon->Start)) : HourMinute(Soon->Start);
   // LOOP-NIGHT-WATCH-20261113: no rest while the night watch is still to come tonight
   const bool bWatchAhead = In.bNightWatch && In.EveningUp > Clock && NightOf(In.EveningUp) == NightOf(Clock);
   Say = TEXT("Too early: the window opens at ") + At + (bWatchAhead ? TEXT(". Do a job until then.") : TEXT(". Do a job or rest until then."));
  }
  else if (In.EveningUp > Clock && NightOf(In.EveningUp) == NightOf(Clock))
   Say = In.bNightWatch ? TEXT("The night watch opens at ") + HourMinute(In.EveningUp) + (In.bSlip ? TEXT(". Until then the time is yours: the slip on the map, the camera, the town.") : TEXT(". Until then the time is yours: the camera, the town."))
    : TEXT("The evening's source is up at ") + HourMinute(In.EveningUp) + TEXT(". Day work until then.");
  // TASKS-TEN-20261113 (change 2 and 3): no "stay in the building until morning" of its own while the next step sends him out with
  // the camera; that the antennas wait for daylight is the half-line after the night's last line, nothing more.
  else if (Hour >= 4 && Hour < 8) Say = TEXT("Late in the shift: finish what is under way. New paid work comes with the morning.");
  if (!Say.IsEmpty() && Clean(In, Say)) N.Clock = Say;
 }
 return N;
}
}

S47TonightNote::FNote S47TonightNote::Gather(const FIn& In, bool bAll) { return S47TonightNoteLocal::BuildNote(In, bAll); }
FString S47TonightNote::KeyOf(const FString& Text) { return S47TonightNoteLocal::Key(S47HudGauges::CardLine(Text)); }
S47TonightNote::FNote S47TonightNote::Build(const FIn& In) { return Gather(In, false); }
S47TonightNote::FDone& S47TonightNote::Tonight() { static FDone Mem; return Mem; }
S47TonightNote::FSeen& S47TonightNote::Seen() { static FSeen Mem; return Mem; }

uint8 S47TonightNote::RankOf(const FString& Source, bool bWindowOpen)
{
 // D45-20261110: by the task's own words, as FS47FreeformSession::Tasks writes them (keep these heads when a line is reworded)
 const FString Up = Source.TrimStartAndEnd().ToUpper();
 if (bWindowOpen || Up.StartsWith(TEXT("FEEDER /")) || Up.StartsWith(TEXT("SERVICE TRUCK / TANK EMPTY")) || Up.StartsWith(TEXT("TRUCK /")) || Up.Contains(TEXT("/ DIAGNOSE AT THE STATUS PC"))) return 0;
 static const TCHAR* const Later[] = { TEXT("STATION GOAL /"), TEXT("ROUND /"), TEXT("MAILBOX /"), TEXT("MAIL ORDER /"), TEXT("OPS / THERE IS A LETTER"), TEXT("OPS / DR. WARD"), TEXT("LEFT BEHIND /"), TEXT("SERVICE WORK ORDER") };
 for (const TCHAR* Head : Later) if (Up.StartsWith(Head)) return 2;
 if (Up.StartsWith(TEXT("CASE /")) && Up.Contains(TEXT("CHOOSE A TEST"))) return 2;
 return 1;
}

bool S47TonightNote::SelfTest(TArray<FString>& Lines)
{
 bool OK = true;
 const auto Check = [&](bool B, const TCHAR* Why) { OK &= B; Lines.Add(FString::Printf(TEXT("%s tonight note: %s"), B ? TEXT("PASS") : TEXT("FAIL"), Why)); };
 const auto Find = [](const FNote& N, const TCHAR* Part) { return N.Lines.IndexOfByPredicate([Part](const FLine& L) { return LineText(L).Contains(Part); }); };
 const auto FindMatter = [](const FNote& N, const TCHAR* Part) { return N.Matters.IndexOfByPredicate([Part](const FLine& L) { return LineText(L).Contains(Part); }); };
 // TASKS-TEN-20261113 (change 9): every row carries giver, window and worth; THE LAND never says a worth
 const auto Tagged = [](const FNote& N)
 {
  bool B = true;
  for (const FLine& L : N.Lines) B &= !TagText(L).IsEmpty() && (L.Giver == TEXT("STATION") || L.Giver == TEXT("WARD") || L.Giver == TEXT("THE LAND")) && (L.Giver != TEXT("THE LAND") || TagText(L).EndsWith(TEXT(" / unknown")));
  for (const FLine& L : N.Matters) B &= !TagText(L).IsEmpty() && !L.Matter.IsEmpty() && (L.Giver != TEXT("THE LAND") || TagText(L).EndsWith(TEXT(" / unknown")));
  return B;
 };
 // Freeform, shift day 2 at 22:39 (Tom's picture, 9 November): the roof test planned for 01:30 to 04:30, film in the lab, a card
 // filed, one up and the night watch to come, a case with no test, a paper from the box not read.
 const double Day2 = 86400.;
 const FString Roof = TEXT("CONTROL ROOM: RETURN TO LISTEN UNDER THE FASTENED ROOF. SHIFT DAY 2 / 01:30 to SHIFT DAY 2 / 04:30 (after midnight). One minute of it must be heard from the control room, in wind like the ribbon's; skipped time is not heard.");
 FIn In;
 In.bFreeform = true;
 In.Clock = Day2 + 22 * 3600. + 39 * 60.;
 // TASKS-TEN-20261113 (change 2): the HUD's ladder chooses the next step; the roof made ready waits for its window
 // (FS47FreeformSession::FieldWindowWaits), so the ladder's step is the film, and the note marks that row
 const FString LabTask = TEXT("PHOTO LAB: DEVELOP, FIX AND COLLECT YOUR EXPOSED PHOTOGRAPH AT THE WET BENCH.");
 In.Next = S47HudGauges::CardLine(LabTask);
 In.NextRow = LabTask;
 In.Tasks = {
  { LabTask, false },
  { TEXT("MAILBOX / One paper you took out of the box is not read yet."), false },
  { TEXT("SARO under review / Dr. Ward assesses the station on Friday, April 25."), false },
  { Roof, false },
  { TEXT("CASE / THE LOST HOURS: choose a control or file your assessment on the OPS case board"), false },
  { TEXT("D02 / FF-D0002-0000 / 3C 286 / FILED"), true },
  { TEXT("D02 / FF-D0002-0001 / CYG A / NEW"), false },
  { TEXT("D02 / FF-D0002-0002 / VIR A / NEW"), false },
 };
 In.Cards = { { TEXT("FF-D0002-0001"), TEXT("CYG A"), -1, Day2 + 23.5 * 3600., 19, true, false }, { TEXT("FF-D0002-0002"), TEXT("VIR A"), Day2 + 86400. + 1.5 * 3600., Day2 + 86400. + 4 * 3600., 19, false, true } };
 In.Windows = { { Roof, Day2 + 86400. + 1.5 * 3600., Day2 + 86400. + 4.5 * 3600. } };
 In.WaitingCases = { TEXT("THE LOST HOURS") };
 In.EveningUp = Day2 + 86400. + 1.5 * 3600.; In.bNightWatch = true;
 FNote N = Build(In);
 const int32 Lab = Find(N, TEXT("PHOTO LAB")), Window = Find(N, TEXT("CONTROL ROOM"));
 Check(Lab != INDEX_NONE && N.Lines[Lab].Mark == EMark::Next && Window != INDEX_NONE && N.Lines[Window].Mark == EMark::Open && TagText(N.Lines[Window]).StartsWith(TEXT("STATION / 01:30-04:30 / "))
  && N.Lines[Window].Kind == 0 && N.Lines[Lab].Kind == 2,
  TEXT("too early for the roof: the film in the lab is the next step (YOUR OWN TIME), the roof's window waits under WORK with its time in its tag"));
 Check(N.Lines.Num() <= MostLines && N.Lines.ContainsByPredicate([](const FLine& L) { return L.Key == TEXT("cards"); }) && Find(N, TEXT("Night watch card")) != INDEX_NONE,
  TEXT("the card has four lines at most: the card up and the night watch to come among them (TASKS-TEN change 6)"));
 Check(N.Clock.StartsWith(TEXT("Too early: the window opens at 01:30.")), TEXT("the clock says it is too early, and when the window opens"));
 Check(N.Also.IsEmpty() && FindMatter(N, TEXT("The Lost Hours")) != INDEX_NONE && FindMatter(N, TEXT("One paper")) != INDEX_NONE && N.Matters.Num() == 2 && !Plain(N).Contains(TEXT("Job cards")),
  TEXT("the case waiting on evidence and the paper not read are open matters (once each), no \"Job cards: N open\" line (TASKS-TEN changes 3 and 6)"));
 Check(N.More == N.Matters.Num() + 0 && Plain(N).Contains(FString::Printf(TEXT("+%d open matters in the notebook"), N.More)), TEXT("one \"+N open matters\" line points to the notebook (TASKS-TEN change 6)"));
 Check(Find(N, TEXT("review")) == INDEX_NONE && Find(N, TEXT("FF-D0002")) == INDEX_NONE, TEXT("the station's standing and the single cards stay off the plan"));
 Check(Plain(N).Contains(TEXT("[>] PHOTO LAB:")) && S47Script::LintText(Plain(N)).IsEmpty() && Tagged(N), TEXT("the note as text marks the next step, every row with giver, window and worth, THE LAND's worth unknown, within the spoiler limits"));
 {
  const FNote Full = Gather(In, true);
  Check(Full.Lines.ContainsByPredicate([](const FLine& L) { return L.Mark == EMark::Done && L.What.Contains(TEXT("registered")); }) && Full.More == 0 && Tagged(Full),
   TEXT("NOW has the card filed tonight ticked off, and everything tagged (TASKS-TEN change 9)"));
 }
 // In the window: the roof is the next step (the ladder's window open now), and the clock says to be there.
 In.Clock = Day2 + 86400. + 2 * 3600.;
 In.Next = S47HudGauges::CardLine(Roof); In.NextRow = Roof;
 N = Build(In);
 const int32 Roofed = Find(N, TEXT("CONTROL ROOM"));
 Check(Roofed != INDEX_NONE && N.Lines[Roofed].Mark == EMark::Next && N.Clock.StartsWith(TEXT("The window is open until 04:30.")) && N.Clock.Contains(TEXT("control room")) && !N.Clock.Contains(TEXT("20 minutes")), TEXT("in the window the roof is the next step, and the clock says until when, and that a minute in the room is what counts (LEAD-FIX-20261110)"));
 // TASKS-TEN-20261113 (change 6): a window shut leaves tonight's list
 In.Clock = Day2 + 86400. + 5 * 3600.; In.NextRow.Reset(); In.Next.Reset();
 Check(Find(Gather(In, true), TEXT("CONTROL ROOM")) == INDEX_NONE, TEXT("a time-boxed row leaves tonight's list when its window has shut (TASKS-TEN change 6)"));
 In.Clock = Day2 + 86400. + 2 * 3600.;
 // Freeform never shows a spoiler, from wherever it comes.
 FIn Bad = In;
 Bad.Tasks.Add({ TEXT("RECORDS ROOM / File the 1947 photograph with the others."), false });
 Bad.Tasks.Add({ TEXT("SERVICE YARD / Tomas left the cable cut at the junction."), false });
 Bad.WaitingCases.Add(TEXT("POINT C"));
 N = Build(Bad);
 Check(S47Script::LintText(Plain(N)).IsEmpty() && !Plain(N).Contains(TEXT("1947")) && !Plain(N).Contains(TEXT("Tomas")), TEXT("a line past the spoiler limits is left off the Freeform note"));
 // Dr. Ward's letter as the next step is the plan's line, not an open matter.
 FIn Letter; Letter.bFreeform = true; Letter.Clock = Day2 + 9 * 3600.;
 Letter.Tasks = { { TEXT("MAILBOX / A letter from Dr. Ward's office is in the box at the road."), false }, { TEXT("MAIL ORDER / A parcel from Western Surplus, due about Friday."), false } };
 Letter.Next = S47HudGauges::CardLine(Letter.Tasks[0].Text);
 N = Build(Letter);
 Check(N.Lines.Num() == 1 && N.Lines[0].Mark == EMark::Next && N.Lines[0].Where == TEXT("MAILBOX") && N.Lines[0].Giver == TEXT("WARD") && N.Also.IsEmpty() && !Plain(N).Contains(TEXT("parcel")),
  TEXT("the letter waiting is the next step at the mailbox, given by WARD; the parcel due is no row (TASKS-TEN changes 3 and 9)"));
 // TASKS-TEN-20261113 (change 3): no status rows: the station's standing, the flag up, a parcel due, YOU / Tired, a card count
 {
  FIn Status; Status.bFreeform = true; Status.Clock = Day2 + 22 * 3600.;
  Status.Tasks = { { TEXT("SARO under review / Dr. Ward assesses the station on Friday, April 25."), false }, { TEXT("MAILBOX / Your order is in the box at the road, the flag up for the carrier."), false },
   { TEXT("MAIL ORDER / A parcel from Western Surplus, due about Friday."), false }, { TEXT("YOU / Tired"), false }, { TEXT("Job cards: 3 open"), false } };
  const FNote S = Gather(Status, true);
  Check(S.Lines.IsEmpty() && S.Also.IsEmpty() && S.Matters.IsEmpty(), TEXT("no status rows: standing, the flag up, a parcel due, YOU / Tired and a card count are never rows (a file's hours are an open matter: change 9) (TASKS-TEN change 3)"));
 }
 // Many things open: four lines at most, and the rest counted for the notebook.
 FIn Many; Many.bFreeform = true; Many.Clock = Day2 + 20 * 3600.;
 for (int32 I = 0; I < 11; ++I) Many.Tasks.Add({ FString::Printf(TEXT("SERVICE YARD / Check spare number %d on the rack."), I + 1), I < 3 });
 Many.Next = S47HudGauges::CardLine(Many.Tasks[3].Text);
 N = Build(Many);
 Check(N.Lines.Num() == MostLines && N.More == 8 - MostLines && Plain(N).Contains(FString::Printf(TEXT("+%d open matters in the notebook"), N.More)), TEXT("a long list keeps to four lines and counts the rest for the notebook (TASKS-TEN change 6)"));
 Check(N.Lines[0].Mark == EMark::Next && N.Lines[0].What.Contains(TEXT("4")) && !N.Lines.ContainsByPredicate([](const FLine& L) { return L.Mark == EMark::Done; }), TEXT("with a full card the next step first, and no room for the things done"));
 // PT-NOTEBOOK-20261109: NOW has the same lines with no limit, so the note's "+N" is exactly what NOW has besides the note.
 {
  const FNote Full = Gather(Many, true);
  int32 FullOpen = 0, NoteOpen = 0, FullDone = 0;
  for (const FLine& L : Full.Lines) { FullOpen += L.Mark != EMark::Done; FullDone += L.Mark == EMark::Done; }
  for (const FLine& L : N.Lines) NoteOpen += L.Mark != EMark::Done;
  bool bAllThere = true;
  for (const FLine& L : N.Lines) bAllThere &= !L.Key.IsEmpty() && Full.Lines.ContainsByPredicate([&L](const FLine& F) { return F.Key == L.Key && F.Mark == L.Mark; });
  Check(Full.More == 0 && Full.Lines.Num() == 11 && FullDone == 3 && FullOpen == NoteOpen + N.More - N.Matters.Num() && bAllThere,
   TEXT("NOW gathers every line with no limit: the note's lines are among them, and \"+N\" counts exactly the rows only NOW shows"));
  Check(Full.Lines.ContainsByPredicate([](const FLine& L) { return L.Source == TEXT("SERVICE YARD / Check spare number 11 on the rack."); }), TEXT("each line keeps the task's own words, for FOLLOW and the details"));
 }
 // PT-NOTEBOOK-20261109: the remote feeder looked at is ticked off and stays struck through until 08:00, also once the list lets it go.
 {
  FDone Mem;
  const FString Feeder = TEXT("WORTH A LOOK / The remote feeder on the county road, after midnight. Check it before noon."); // D45-20261110: as the Director words it
  FIn Trail; Trail.bFreeform = true; Trail.Done = &Mem; Trail.Clock = Day2 + 86400. + 1.2 * 3600.;
  Trail.Tasks = { { Feeder, false }, { TEXT("PHOTO LAB: DEVELOP, FIX AND COLLECT YOUR EXPOSED PHOTOGRAPH AT THE WET BENCH."), false } };
  FNote T = Gather(Trail, true);
  const int32 Open = Find(T, TEXT("remote feeder"));
  Check(Open != INDEX_NONE && T.Lines[Open].Mark != EMark::Done && T.Lines[Open].Kind == 1 && TagText(T.Lines[Open]) == TEXT("THE LAND / until noon / unknown"), TEXT("the feeder worth a look is open before the visit, a FOLLOW-UP of THE LAND's, until noon, worth unknown"));
  Trail.Tasks[0].bDone = true; Trail.Clock += 1800;
  T = Gather(Trail, true);
  const int32 Ticked = Find(T, TEXT("remote feeder"));
  Check(Ticked != INDEX_NONE && T.Lines[Ticked].Mark == EMark::Done, TEXT("after the visit the feeder's line is ticked off and struck"));
  Trail.Tasks.RemoveAt(0); Trail.Clock += 1800;
  T = Gather(Trail, true);
  const FNote Small = Gather(Trail);
  const int32 Kept = Find(T, TEXT("remote feeder")), KeptNote = Find(Small, TEXT("remote feeder"));
  Check(Kept != INDEX_NONE && T.Lines[Kept].Mark == EMark::Done && KeptNote != INDEX_NONE && Small.Lines[KeptNote].Mark == EMark::Done, TEXT("a line done tonight stays struck on NOW and on the note when the list lets it go"));
  Trail.Clock = Day2 + 86400. + 8.5 * 3600.;
  T = Gather(Trail, true);
  Check(Find(T, TEXT("remote feeder")) == INDEX_NONE, TEXT("at 08:00 the night's struck lines are let go"));
  Trail.Clock = Day2 + 86400. + 2 * 3600.; Trail.Tasks.Insert({ Feeder, true }, 0); Gather(Trail, true);
  Trail.Tasks.RemoveAt(0); Trail.Game = 7; Trail.Clock += 600;
  Check(Find(Gather(Trail, true), TEXT("remote feeder")) == INDEX_NONE, TEXT("another game loaded the same night forgets what this one did"));
  Check(S47Script::LintText(Plain(T)).IsEmpty(), TEXT("the remembered lines keep to the spoiler limits"));
 }
 // D45-20261110 (Tom: a plain list, by importance), TASKS-TEN-20261113 (change 6): by group first, WORK (the feeder), FOLLOW-UP (the
 // slip), YOUR OWN TIME (the round), whatever the list's order; one sentence on every surface, the whole words kept in the source.
 {
  const FString Slip = TEXT("BEARING SLIP / New circle on your map, 1.1 miles south-southwest of SARO. Drive out and look before noon.");
  Check(RankOf(TEXT("FEEDER / Measure, isolate and replace the control fuse; restore SARO power."), false) == 0 && RankOf(Slip, false) == 1
   && RankOf(TEXT("STATION GOAL / ARRAY 19 OF 27: REPAIR THE WEST BRANCH AT ANTENNA S-13. YOU HAVE THE PARTS."), false) == 2 && RankOf(TEXT("PHOTO LAB: DEVELOP"), true) == 0
   && RankOf(TEXT("S-13 / Diagnose at the status PC, replace the relay and run the local out-and-back test."), false) == 0 && RankOf(TEXT("ROUND / Today: the tank."), false) == 2,
   TEXT("DO FIRST the feeder and a dish's fault, TONIGHT the bearing slip, WHEN YOU CAN the station goal and the round; a window open now is first"));
  FIn Order; Order.bFreeform = true; Order.Clock = Day2 + 21 * 3600.;
  for (int32 I = 0; I < 7; ++I) Order.Tasks.Add({ FString::Printf(TEXT("ROUND / Today: stop number %d. $6 on the sheet when it is done."), I + 1), false });
  Order.Tasks.Add({ Slip, false });
  Order.Tasks.Add({ TEXT("FEEDER / Measure, isolate and replace the control fuse; restore SARO power."), false });
  const FNote Short = Build(Order), Long = Gather(Order, true);
  const int32 Feed = Find(Short, TEXT("FEEDER")), Bear = Find(Short, TEXT("New circle")), Stop = Find(Short, TEXT("stop number 1"));
  const int32 LongSlip = Find(Long, TEXT("New circle"));
  Check(Short.Lines.Num() == MostLines && Feed == 0 && Bear == 1 && (Stop == INDEX_NONE || Stop > Bear) && Short.Lines[Feed].Kind == 0 && Short.Lines[Bear].Kind == 1 && Short.More == 9 - MostLines
   && Stop != INDEX_NONE && TagText(Short.Lines[Stop]) == TEXT("STATION / until 19:00 / $6"),
   TEXT("the four-line card puts WORK before FOLLOW-UP before YOUR OWN TIME, whatever the list's order, and counts the rest; the round says its pay"));
  Order.Tasks[7].bDone = true;
  const FNote Ticked = Gather(Order, true);
  const int32 TickedSlip = Find(Ticked, TEXT("New circle"));
  Check(LongSlip != INDEX_NONE && Long.Lines[LongSlip].What == TEXT("New circle on your map, 1.1 miles south-southwest of SARO.") && Long.Lines[LongSlip].Source == Slip && Short.Lines[Bear].What == Long.Lines[LongSlip].What
   && TickedSlip != INDEX_NONE && Ticked.Lines[TickedSlip].Mark == EMark::Done && Ticked.Lines[TickedSlip].Key == Long.Lines[LongSlip].Key && S47Script::LintText(Plain(Long)).IsEmpty(),
   TEXT("one sentence on the card and on NOW, the slip's whole words kept for NOW's WHY; ticked off it keeps its key (TASKS-TEN change 9)"));
 }
 // LOOP-NIGHT-WATCH-20261113: an ordinary night after the two early cards. From 20:30 to the watch at 01:30 the hours are his own: the
 // note says when the watch opens and what is open till then, never rest; once the watch is up the old evening line is not said.
 {
  FIn Gap; Gap.bFreeform = true; Gap.EveningUp = Day2 + 86400. + 1.5 * 3600.; Gap.bNightWatch = true; Gap.bSlip = true;
  Gap.Tasks = { { TEXT("D02 / FF-D0002-0002 / VIR A / NEW"), false } };
  bool bWatchSaid = true, bNoRest = true;
  for (const double H : { 20.5, 22., 23.75, 24.5, 25.25 })
  {
   Gap.Clock = Day2 + H * 3600.;
   const FNote G = Build(Gap);
   bWatchSaid &= G.Clock.StartsWith(TEXT("The night watch opens at 01:30. Until then the time is yours: the slip on the map"));
   bNoRest &= !G.Clock.Contains(TEXT("rest")) && !Plain(G).Contains(TEXT("rest until")) && S47Script::LintText(Plain(G)).IsEmpty();
  }
  Gap.bSlip = false; Gap.Clock = Day2 + 23 * 3600.;
  const FNote NoSlip = Build(Gap);
  Gap.Windows = { { TEXT("CONTROL ROOM: LISTEN UNDER THE ROOF."), Day2 + 86400. + 1. * 3600., Day2 + 86400. + 3. * 3600. } };
  const FNote CaseFirst = Build(Gap);
  Gap.Windows.Reset(); Gap.Clock = Day2 + 86400. + 1.75 * 3600.;
  const FNote Up = Build(Gap);
  Check(bWatchSaid && bNoRest && NoSlip.Clock.EndsWith(TEXT("the time is yours: the camera, the town.")) && CaseFirst.Clock.EndsWith(TEXT("Do a job until then.")) && !Up.Clock.Contains(TEXT("night watch opens")),
   TEXT("20:30 to 01:30: the night watch's hour and the slip, the camera and the town, never rest (a case window too); once it is up the note stops saying it"));
 }
 // TASKS-TEN-20261113 (change 4): the cards counted one way: those up on one row with the HUD's count, the night watch not up named with
 // its window and never counted open; the HUD's row is marked Next in the HUD's own words
 {
  FIn Cards; Cards.bFreeform = true; Cards.Clock = Day2 + 22 * 3600.;
  Cards.Cards = { { TEXT("FF-D0002-0000"), TEXT("3C 286"), -1, Day2 + 23.5 * 3600., 19, true, false }, { TEXT("FF-D0002-0001"), TEXT("CYG A"), -1, Day2 + 23.5 * 3600., 19, true, false },
   { TEXT("FF-D0002-0002"), TEXT("VIR A"), Day2 + 86400. + 1.5 * 3600., Day2 + 86400. + 4 * 3600., 19, false, true } };
  Cards.Tasks = { { TEXT("D02 / FF-D0002-0000 / 3C 286 / NEW"), false }, { TEXT("D02 / FF-D0002-0001 / CYG A / NEW"), false }, { TEXT("D02 / FF-D0002-0002 / VIR A / NEW"), false } };
  Cards.Next = TEXT("OPS // 2 JOB CARDS UP."); Cards.NextRow = TEXT("cards");
  const FNote C = Gather(Cards, true);
  const int32 UpRow = Find(C, TEXT("2 job cards up")), WatchRow = Find(C, TEXT("Night watch card"));
  Check(UpRow != INDEX_NONE && C.Lines[UpRow].Mark == EMark::Next && C.Lines[UpRow].Key == TEXT("cards") && TagText(C.Lines[UpRow]) == TEXT("STATION / until 23:30 / $38")
   && WatchRow != INDEX_NONE && TagText(C.Lines[WatchRow]) == TEXT("STATION / 01:30-04:00 / $19") && Find(C, TEXT("3 job cards")) == INDEX_NONE && !Plain(C).Contains(TEXT("open at OPS")),
   TEXT("the cards up on one row as the HUD counts them (2, $38 until 23:30), the night watch named with its window and pay, never counted (TASKS-TEN changes 4 and 9)"));
 }
 // TASKS-TEN-20261113 (change 9): OPEN MATTERS by group, Cases, Station, Post, Unknowns, Ward; no ticks; the station's one row given by
 // the caller stands for the goal's and the work order's lines; a file's hours are an unknown, the weekly report Ward's
 {
  FIn Open; Open.bFreeform = true; Open.Clock = Day2 + 23 * 3600.;
  const FString Goal = TEXT("STATION GOAL / ARRAY 19 OF 27: REPAIR THE WEST BRANCH AT ANTENNA S-13 (+3 ANTENNAS). NEEDS 1 RELAY.");
  Open.Tasks = { { Goal, false }, { TEXT("UNKNOWN 2 / Logged so far at 21:00, 02:00."), false }, { TEXT("CASE / THE SAME MINUTE: choose a test on the OPS case board"), false },
   { TEXT("OPS / There is a letter from Dr. Ward's office in the tray."), false } };
  Open.StationRows = { Goal };
  FLine Station; Station.Matter = TEXT("Station"); Station.Where = TEXT("ARRAY STATUS"); Station.What = TEXT("Array 19 of 27: next the west branch at S-13, needs 1 relay."); Station.Key = TEXT("matter:station");
  Station.Giver = TEXT("STATION"); Station.Span = TEXT("by daylight"); Station.Worth = TEXT("+3 antennas");
  FLine Week; Week.Matter = TEXT("Ward"); Week.Where = TEXT("OPS"); Week.What = TEXT("The weekly sheet goes to Dr. Ward on day 8."); Week.Key = TEXT("matter:week");
  Week.Giver = TEXT("WARD"); Week.Span = TEXT("by day 8"); Week.Worth = TEXT("counts for the review");
  Open.Matters = { Week, Station };
  const FNote O = Gather(Open, true);
  TArray<FString> Groups; for (const FLine& L : O.Matters) Groups.Add(L.Matter);
  Check(O.Lines.IsEmpty() && Groups == TArray<FString>({ TEXT("Cases"), TEXT("Station"), TEXT("Post"), TEXT("Unknowns"), TEXT("Ward") }) && Tagged(O)
   && O.Matters[1].What.Contains(TEXT("needs 1 relay")) && TagText(O.Matters[4]) == TEXT("WARD / by day 8 / counts for the review") && TagText(O.Matters[3]).EndsWith(TEXT(" / unknown"))
   && !O.Matters.ContainsByPredicate([](const FLine& L) { return L.Mark == EMark::Done; }),
   TEXT("OPEN MATTERS: Cases, Station (one row for the chain), Post, Unknowns, Ward, each tagged, no ticks; the goal without parts is no row tonight (TASKS-TEN change 9)"));
 }
 // Story (chapter 2): the list's own order, the first thing not done is next with the objective's place; Story's words are its own.
 FIn Story;
 Story.Clock = 2 * 3600.;
 Story.Next = TEXT("SARO RECORDS // COMPARE THE ORIGINAL AND AMENDED FIELD RECORDS ON THE CASE BOARD");
 Story.Tasks = {
  { TEXT("Go to the control room and answer the supervisor's call"), true },
  { TEXT("Find the reference record (south corridor)"), true },
  { TEXT("Read the 1947 field record (E07)"), false },
  { TEXT("Read the amended service copy (E06)"), false },
 };
 N = Build(Story);
 Check(N.Lines.Num() == 4 && N.Lines[0].Mark == EMark::Done && N.Lines[0].Where == TEXT("CONTROL ROOM") && N.Lines[1].Where == TEXT("CORRIDOR")
  && N.Lines[2].Mark == EMark::Next && N.Lines[2].Where == TEXT("RECORDS") && N.Lines[2].What.Contains(TEXT("1947")) && N.Lines[3].Mark == EMark::Open,
  TEXT("Story: done ticked in order, the next with the objective's room, Story's own words kept"));
 Check(N.Also.IsEmpty() && N.Clock.IsEmpty() && N.More == 0 && N.Matters.IsEmpty() && TagText(N.Lines[2]).IsEmpty(), TEXT("Story says nothing of cards, post, windows or givers it does not have"));
 Check(HourMinute(Day2 + 86400. + 1.5 * 3600.) == TEXT("01:30") && HourMinute(-1) == TEXT("--:--"), TEXT("the hour as the note writes it"));
 return OK;
}

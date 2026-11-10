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
 bool IsStatus(const FString& T)
 {
  return T.StartsWith(TEXT("SARO under review")) || T.StartsWith(TEXT("SARO KEPT OPEN")) || T.StartsWith(TEXT("SARO CLOSES"));
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
 void Split(const FString& Text, FString& Where, FString& What)
 {
  const FString Card = S47HudGauges::CardLine(Text, 1000);
  FString Head, Body;
  if (Card.Split(TEXT(" // "), &Head, &Body)) { Where = Sign(Head); What = Sentences(Body, MostChars); }
  else What = Sentences(Card, MostChars);
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
 FString Out = Line.When.IsEmpty() ? FString() : Line.When + TEXT("  ");
 if (!Line.Where.IsEmpty()) Out += Line.Where + TEXT(": ");
 return Out + Line.What;
}

FString S47TonightNote::Plain(const FNote& Note)
{
 TArray<FString> Out;
 for (const FLine& L : Note.Lines) Out.Add((L.Mark == EMark::Done ? TEXT("[x] ") : L.Mark == EMark::Next ? TEXT("[>] ") : TEXT("[ ] ")) + LineText(L));
 if (Note.More > 0) Out.Add(FString::Printf(TEXT("+%d more in the notebook"), Note.More));
 for (const FString& A : Note.Also) Out.Add(A);
 if (!Note.Clock.IsEmpty()) Out.Add(Note.Clock);
 return FString::Join(Out, TEXT("\n"));
}

namespace S47TonightNoteLocal
{
FNote BuildNote(const FIn& In, bool bAll)
{
 FNote N;
 TArray<FWork> All;
 const FString NextKey = Key(S47HudGauges::CardLine(In.Next));
 auto IsNextKey = [&NextKey](const FString& K) { return !NextKey.IsEmpty() && (K == NextKey || (K.Len() >= 12 && NextKey.StartsWith(K))); };
 int32 Filed = 0, Open = 0;
 TArray<FString> Post;
 for (int32 I = 0; I < In.Tasks.Num(); ++I)
 {
  const FString Text = In.Tasks[I].Text.TrimStartAndEnd();
  const bool bDone = In.Tasks[I].bDone;
  if (Text.IsEmpty()) continue;
  if (In.bFreeform)
  {
   // the job cards are counted, not listed: they are the OPS list's own
   if (IsJob(Text)) { (bDone ? Filed : Open) += 1; continue; }
   // (the post is "also open", unless it is the next thing to do: Dr. Ward's letter after the first shift)
   if (IsPost(Text) && !IsNextKey(Key(S47HudGauges::CardLine(Text)))) { if (!bDone) { FString Rest; Text.Split(TEXT(" / "), nullptr, &Rest); Post.Add(Rest.IsEmpty() ? Text : Rest); } continue; }
   if (IsStatus(Text)) continue;
   if (In.WaitingCases.ContainsByPredicate([&Text](const FString& Title) { return !Title.IsEmpty() && Text.StartsWith(TEXT("CASE / ") + Title); })) continue;
  }
  FWork W;
  W.L.Mark = bDone ? EMark::Done : EMark::Open;
  W.Order = I;
  W.Key = Key(S47HudGauges::CardLine(Text));
  W.L.Source = Text;
  Split(Text, W.L.Where, W.L.What);
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
  Split(Wd.Text, W.L.Where, W.L.What);
  All.Add(W);
 }
 // tonight's filed cards, ticked off on one line
 if (Filed > 0)
 {
  FWork W; W.L.Mark = EMark::Done; W.Order = -2; W.L.Where = TEXT("OPS"); W.Key = TEXT("filed cards"); W.L.Source = W.Key;
  W.L.What = Filed == 1 ? FString(TEXT("One job card registered and filed.")) : FString::Printf(TEXT("%d job cards registered and filed."), Filed);
  All.Insert(W, 0);
 }
 for (FWork& W : All) if (W.Windowed()) W.L.When = Window(W.Start, W.End, In.Clock, In.bFreeform);
 // the spoiler limits before anything is chosen, so that what is counted is what could be shown
 All.RemoveAll([&In](const FWork& W) { return !Clean(In, LineText(W.L)); });
 for (FWork& W : All) W.L.Key = W.Key;
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

 // The next step. A window open now comes first: be there for it. Otherwise the objective's own line; in Story, whose list says it
 // in other words, the first thing not done, with the objective's place.
 const double Clock = In.Clock;
 auto IsOpen = [](const FWork& W) { return W.L.Mark != EMark::Done; };
 int32 Next = All.IndexOfByPredicate([&](const FWork& W) { return IsOpen(W) && W.Windowed() && Clock >= W.Start && Clock < W.End; });
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
  Split(In.Next, W.L.Where, W.L.What);
  if (!W.L.What.IsEmpty() && Clean(In, LineText(W.L))) { All.Add(W); Next = All.Num() - 1; }
 }
 // a window that has not opened is no next step while there is something to do before it
 if (Next != INDEX_NONE && All[Next].Windowed() && Clock < All[Next].Start)
 {
  const int32 Before = All.IndexOfByPredicate([&](const FWork& W) { return IsOpen(W) && !W.Windowed(); });
  if (Before != INDEX_NONE) Next = Before;
 }

 // What there is room for: the next step, the windows (the night's appointments), the rest in the list's order, and the last
 // things done (two, or one when the open lines fill the note).
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
 int32 DoneShown = bAll ? Done.Num() : FMath::Min(Done.Num(), MostDone);
 if (!bAll && HasNext + OpenCount + DoneShown > MostLines) DoneShown = FMath::Min(Done.Num(), 1);
 int32 Room = bAll ? OpenCount : FMath::Max(0, MostLines - HasNext - DoneShown);
 const int32 WindowsShown = FMath::Min(Windows.Num(), Room); Room -= WindowsShown;
 const int32 RestShown = FMath::Min(Rest.Num(), Room);
 N.More = OpenCount - WindowsShown - RestShown;
 for (int32 I = Done.Num() - DoneShown; I < Done.Num(); ++I) N.Lines.Add(Done[I].L);
 if (HasNext) { FLine L = All[Next].L; L.Mark = EMark::Next; N.Lines.Add(L); }
 for (int32 I = 0; I < RestShown; ++I) N.Lines.Add(Rest[I].L);
 for (int32 I = 0; I < WindowsShown; ++I) N.Lines.Add(Windows[I].L);

 if (!In.bFreeform) return N;

 // What else is open: the job cards, the cases waiting on evidence, the post.
 TArray<FString> Also, AlsoKeys;
 if (Open > 0)
 {
  FString Cards = Open == 1 ? FString(TEXT("Job cards: one open at OPS")) : FString::Printf(TEXT("Job cards: %d open at OPS"), Open);
  if (In.EveningUp >= 0 && (Clock < 0 || (In.EveningUp > Clock && NightOf(In.EveningUp) == NightOf(Clock)))) Cards += TEXT("; the evening's source is up at ") + HourMinute(In.EveningUp);
  Also.Add(Cards + TEXT(".")); AlsoKeys.Add(TEXT("cards"));
 }
 if (In.WaitingCases.Num())
 {
  TArray<FString> Titles;
  for (const FString& T : In.WaitingCases) Titles.Add(S47HudGauges::Handwritten(T, true));
  Also.Add((Titles.Num() == 1 ? TEXT("Case waiting on evidence: ") : TEXT("Cases waiting on evidence: ")) + FString::Join(Titles, TEXT(", ")) + TEXT(".")); AlsoKeys.Add(TEXT("cases"));
 }
 if (Post.Num()) { Also.Add(TEXT("Post: ") + Post[0] + (Post.Num() > 1 ? FString::Printf(TEXT(" (+%d more)"), Post.Num() - 1) : FString())); AlsoKeys.Add(TEXT("post")); }
 for (int32 I = 0; I < Also.Num(); ++I) if (Clean(In, Also[I])) { N.Also.Add(Also[I]); N.AlsoKeys.Add(AlsoKeys[I]); }

 // What the clock allows now.
 if (Clock >= 0)
 {
  const FWork* Now = All.FindByPredicate([&](const FWork& W) { return IsOpen(W) && W.Windowed() && Clock >= W.Start && Clock < W.End; });
  const FWork* Soon = nullptr;
  for (const FWork& W : All) if (IsOpen(W) && W.Windowed() && W.Start > Clock && (!Soon || W.Start < Soon->Start)) Soon = &W;
  const double Hour = FMath::Fmod(Clock, Day) / 3600.;
  FString Say;
  // LEAD-FIX-20261110: as the gates in FS47CaseFieldTests have it: the roof wants a minute heard in the control room; the gate and the
  // field post count 20 minutes skipped (FS47FreeformCases::UnwatchedAllowance); a job's window simply has to be worked in.
  if (Now) Say = TEXT("The window is open until ") + HourMinute(Now->End) + (Now->L.Source.StartsWith(TEXT("CONTROL ROOM")) ? TEXT(". One minute of it heard in the control room will do; skipped time is not heard.")
   : Now->L.Source.Contains(TEXT("SERVICE GATE")) || Now->L.Source.Contains(TEXT("FIELD POST")) ? TEXT(". Up to 20 minutes of it may be skipped or slept; more, and it records nothing.") : TEXT(". Time skipped past it records nothing."));
  else if (Soon)
  {
   const FString At = NightOf(Soon->Start) != NightOf(Clock) ? FString::Printf(TEXT("%s on shift day %d"), *HourMinute(Soon->Start), NightOf(Soon->Start)) : HourMinute(Soon->Start);
   Say = TEXT("Too early: the window opens at ") + At + TEXT(". Do a job or rest until then.");
  }
  else if (In.EveningUp > Clock && NightOf(In.EveningUp) == NightOf(Clock)) Say = TEXT("The evening's source is up at ") + HourMinute(In.EveningUp) + TEXT(". Day work until then.");
  else if (In.bAntennasWait) Say = TEXT("The antennas wait for daylight. Stay in the building until morning.");
  else if (Hour >= 4 && Hour < 8) Say = TEXT("Late in the shift: finish what is under way. New paid work comes with the morning.");
  if (!Say.IsEmpty() && Clean(In, Say)) N.Clock = Say;
 }
 return N;
}
}

S47TonightNote::FNote S47TonightNote::Gather(const FIn& In, bool bAll) { return S47TonightNoteLocal::BuildNote(In, bAll); }
S47TonightNote::FNote S47TonightNote::Build(const FIn& In) { return Gather(In, false); }
S47TonightNote::FDone& S47TonightNote::Tonight() { static FDone Mem; return Mem; }

bool S47TonightNote::SelfTest(TArray<FString>& Lines)
{
 bool OK = true;
 const auto Check = [&](bool B, const TCHAR* Why) { OK &= B; Lines.Add(FString::Printf(TEXT("%s tonight note: %s"), B ? TEXT("PASS") : TEXT("FAIL"), Why)); };
 const auto Find = [](const FNote& N, const TCHAR* Part) { return N.Lines.IndexOfByPredicate([Part](const FLine& L) { return LineText(L).Contains(Part); }); };
 // Freeform, shift day 2 at 22:39 (Tom's picture, 9 November): the roof test planned for 01:30 to 04:30, film in the lab, a card
 // filed and two open (one the evening's), a case with no test, a paper from the box not read.
 const double Day2 = 86400.;
 const FString Roof = TEXT("CONTROL ROOM: RETURN TO LISTEN UNDER THE FASTENED ROOF. SHIFT DAY 2 / 01:30 to SHIFT DAY 2 / 04:30 (after midnight). One minute of it must be heard from the control room, in wind like the ribbon's; skipped time is not heard.");
 FIn In;
 In.bFreeform = true;
 In.Clock = Day2 + 22 * 3600. + 39 * 60.;
 In.Next = S47HudGauges::CardLine(Roof);
 In.Tasks = {
  { TEXT("PHOTO LAB: DEVELOP, FIX AND COLLECT YOUR EXPOSED PHOTOGRAPH AT THE WET BENCH."), false },
  { TEXT("MAILBOX / One paper you took out of the box is not read yet."), false },
  { TEXT("SARO under review / Dr. Ward assesses the station on Friday, April 25."), false },
  { Roof, false },
  { TEXT("CASE / THE LOST HOURS: choose a control or file your assessment on the OPS case board"), false },
  { TEXT("D02 / FF-D0002-0000 / 3C 286 / FILED"), true },
  { TEXT("D02 / FF-D0002-0001 / CYG A / NEW"), false },
  { TEXT("D02 / FF-D0002-0002 / VIR A / NEW"), false },
 };
 In.Windows = { { Roof, Day2 + 86400. + 1.5 * 3600., Day2 + 86400. + 4.5 * 3600. } };
 In.WaitingCases = { TEXT("THE LOST HOURS") };
 In.EveningUp = Day2 + 23 * 3600.;
 FNote N = Build(In);
 const int32 Lab = Find(N, TEXT("PHOTO LAB")), Window = Find(N, TEXT("01:30-04:30  CONTROL ROOM"));
 Check(Lab != INDEX_NONE && N.Lines[Lab].Mark == EMark::Next && Window != INDEX_NONE && N.Lines[Window].Mark == EMark::Open && Lab < Window,
  TEXT("too early for the roof: the film in the lab is the next step, the roof's window waits on the note with its time and room"));
 Check(N.Lines.Num() && N.Lines[0].Mark == EMark::Done && N.Lines[0].What.Contains(TEXT("registered")), TEXT("the card filed tonight is ticked off at the top"));
 Check(N.Clock.StartsWith(TEXT("Too early: the window opens at 01:30.")), TEXT("the clock says it is too early, and when the window opens"));
 Check(N.Also.Num() == 3 && N.Also[0].Contains(TEXT("2 open at OPS")) && N.Also[0].Contains(TEXT("23:00")) && N.Also[1].Contains(TEXT("The Lost Hours")) && N.Also[2].StartsWith(TEXT("Post: One paper")),
  TEXT("also open: the job cards with the evening's hour, the case waiting on evidence, the paper not read"));
 Check(Find(N, TEXT("review")) == INDEX_NONE && Find(N, TEXT("FF-D0002")) == INDEX_NONE, TEXT("the station's standing and the single cards stay off the plan"));
 Check(Plain(N).Contains(TEXT("[>] PHOTO LAB:")) && Plain(N).Contains(TEXT("[x] OPS:")) && S47Script::LintText(Plain(N)).IsEmpty(), TEXT("the note as text marks done and next, and keeps to the spoiler limits"));
 // In the window: the roof is the next step, and the clock says to be there.
 In.Clock = Day2 + 86400. + 2 * 3600.;
 N = Build(In);
 const int32 Roofed = Find(N, TEXT("CONTROL ROOM"));
 Check(Roofed != INDEX_NONE && N.Lines[Roofed].Mark == EMark::Next && N.Clock.StartsWith(TEXT("The window is open until 04:30.")) && N.Clock.Contains(TEXT("control room")) && !N.Clock.Contains(TEXT("20 minutes")), TEXT("in the window the roof is the next step, and the clock says until when, and that a minute in the room is what counts (LEAD-FIX-20261110)"));
 // Freeform never shows a spoiler, from wherever it comes.
 FIn Bad = In;
 Bad.Tasks.Add({ TEXT("RECORDS ROOM / File the 1947 photograph with the others."), false });
 Bad.Tasks.Add({ TEXT("SERVICE YARD / Tomas left the cable cut at the junction."), false });
 Bad.WaitingCases.Add(TEXT("POINT C"));
 N = Build(Bad);
 Check(S47Script::LintText(Plain(N)).IsEmpty() && !Plain(N).Contains(TEXT("1947")) && !Plain(N).Contains(TEXT("Tomas")), TEXT("a line past the spoiler limits is left off the Freeform note"));
 // Dr. Ward's letter as the next step is the plan's line, not the post's.
 FIn Letter; Letter.bFreeform = true; Letter.Clock = Day2 + 9 * 3600.;
 Letter.Tasks = { { TEXT("MAILBOX / A letter from Dr. Ward's office is in the box at the road."), false }, { TEXT("MAIL ORDER / A parcel from Western Surplus, due about Friday."), false } };
 Letter.Next = S47HudGauges::CardLine(Letter.Tasks[0].Text);
 N = Build(Letter);
 Check(N.Lines.Num() == 1 && N.Lines[0].Mark == EMark::Next && N.Lines[0].Where == TEXT("MAILBOX") && N.Also.Num() == 1 && N.Also[0].Contains(TEXT("parcel")), TEXT("the letter waiting is the next step at the mailbox; the parcel is also open"));
 // Many things open: six lines at most, and the rest counted.
 FIn Many; Many.bFreeform = true; Many.Clock = Day2 + 20 * 3600.;
 for (int32 I = 0; I < 11; ++I) Many.Tasks.Add({ FString::Printf(TEXT("SERVICE YARD / Check spare number %d on the rack."), I + 1), I < 3 });
 Many.Next = S47HudGauges::CardLine(Many.Tasks[3].Text);
 N = Build(Many);
 Check(N.Lines.Num() == MostLines && N.More == 7 - (MostLines - 2) && Plain(N).Contains(FString::Printf(TEXT("+%d more in the notebook"), N.More)), TEXT("a long list keeps to six lines and counts the rest for the notebook"));
 Check(N.Lines[0].Mark == EMark::Done && N.Lines[1].Mark == EMark::Next && N.Lines[1].What.Contains(TEXT("4")), TEXT("with a full note one thing done stays ticked, then the next step"));
 // PT-NOTEBOOK-20261109: NOW has the same lines with no limit, so the note's "+N more" is exactly what NOW has besides the note.
 {
  const FNote Full = Gather(Many, true);
  int32 FullOpen = 0, NoteOpen = 0, FullDone = 0;
  for (const FLine& L : Full.Lines) { FullOpen += L.Mark == EMark::Open; FullDone += L.Mark == EMark::Done; }
  for (const FLine& L : N.Lines) NoteOpen += L.Mark == EMark::Open;
  bool bAllThere = true;
  for (const FLine& L : N.Lines) bAllThere &= !L.Key.IsEmpty() && Full.Lines.ContainsByPredicate([&L](const FLine& F) { return F.Key == L.Key && F.Mark == L.Mark; });
  Check(Full.More == 0 && Full.Lines.Num() == 11 && FullDone == 3 && FullOpen == NoteOpen + N.More && bAllThere,
   TEXT("NOW gathers every line with no limit: the note's lines are among them, and \"+N more\" counts exactly the open lines only NOW shows"));
  Check(Full.Lines.ContainsByPredicate([](const FLine& L) { return L.Source == TEXT("SERVICE YARD / Check spare number 11 on the rack."); }), TEXT("each line keeps the task's own words, for FOLLOW and the details"));
 }
 // PT-NOTEBOOK-20261109: the remote feeder looked at is ticked off and stays struck through until 08:00, also once the list lets it go.
 {
  FDone Mem;
  const FString Feeder = TEXT("WORTH A LOOK / The remote feeder on the county road, after midnight. Gone by noon.");
  FIn Trail; Trail.bFreeform = true; Trail.Done = &Mem; Trail.Clock = Day2 + 86400. + 1.2 * 3600.;
  Trail.Tasks = { { Feeder, false }, { TEXT("PHOTO LAB: DEVELOP, FIX AND COLLECT YOUR EXPOSED PHOTOGRAPH AT THE WET BENCH."), false } };
  FNote T = Gather(Trail, true);
  const int32 Open = Find(T, TEXT("remote feeder"));
  Check(Open != INDEX_NONE && T.Lines[Open].Mark != EMark::Done, TEXT("the feeder worth a look is open before the visit"));
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
 Check(N.Also.IsEmpty() && N.Clock.IsEmpty() && N.More == 0, TEXT("Story says nothing of cards, post or windows it does not have"));
 Check(HourMinute(Day2 + 86400. + 1.5 * 3600.) == TEXT("01:30") && HourMinute(-1) == TEXT("--:--"), TEXT("the hour as the note writes it"));
 return OK;
}

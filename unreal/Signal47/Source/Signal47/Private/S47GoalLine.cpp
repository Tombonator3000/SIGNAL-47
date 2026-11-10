#include "S47GoalLine.h"
#include "S47Script.h"

// N1110 (N1110-9): see S47GoalLine.h.
namespace
{
 FString Clean(const FString& Line)
 {
  if (S47Script::LintText(Line).IsEmpty()) return Line;
  UE_LOG(LogTemp, Display, TEXT("S47: a goal line is left off (%s)"), *Line);
  return FString();
 }
 // the first sentence alone, at most 110 characters
 FString OneSentence(const FString& Text)
 {
  const FString Body = Text.TrimStartAndEnd();
  for (int32 I = 0; I + 1 < Body.Len(); ++I)
   if ((Body[I] == TEXT('?') || Body[I] == TEXT('.') || Body[I] == TEXT('!')) && Body[I + 1] == TEXT(' ') && !Body.Left(I + 1).EndsWith(TEXT("Dr.")) && !Body.Left(I + 1).EndsWith(TEXT("Mr."))) return S47GoalLine::FirstSentence(Body.Left(I + 1), 110);
  return S47GoalLine::FirstSentence(Body, 110);
 }
 // runs of blanks become one (the schedule's lines are set with two)
 FString Squash(FString Text)
 {
  Text = Text.TrimStartAndEnd();
  while (Text.Contains(TEXT("  "))) Text.ReplaceInline(TEXT("  "), TEXT(" "));
  return Text;
 }
 FString Sentence(const FString& Text)
 {
  FString T = Text.TrimStartAndEnd();
  if (T.IsEmpty() || T.EndsWith(TEXT(".")) || T.EndsWith(TEXT("?")) || T.EndsWith(TEXT("!"))) return T;
  return T + TEXT(".");
 }
}

FString S47GoalLine::FirstSentence(const FString& Text, int32 Most)
{
 const FString Body = Text.TrimStartAndEnd();
 FString Out;
 int32 From = 0;
 for (int32 I = 0; I < Body.Len(); ++I)
 {
  const TCHAR Ch = Body[I];
  const bool bEnd = I + 1 == Body.Len() || ((Ch == TEXT('.') || Ch == TEXT('?') || Ch == TEXT('!')) && Body[I + 1] == TEXT(' '));
  if (!bEnd) continue;
  const FString Piece = Body.Mid(From, I + 1 - From).TrimStartAndEnd();
  if (Piece.EndsWith(TEXT("Dr.")) || Piece.EndsWith(TEXT("Mr."))) continue; // (N1110: From stays, so "Dr. Smith called." stays whole)
  From = I + 1;
  if (Out.IsEmpty()) { Out = Piece; if (Out.Len() >= Most) break; }
  else if (Out.Len() + 1 + Piece.Len() <= Most) Out += TEXT(" ") + Piece;
  else break;
 }
 if (Out.Len() > Most) Out = Out.Left(Most - 3).TrimEnd() + TEXT("...");
 return Out;
}

TArray<FString> S47GoalLine::FGoals::Lines() const
{
 TArray<FString> Out;
 for (const FString* L : { &Tonight, &Week, &Question }) if (!L->IsEmpty()) Out.Add(*L);
 return Out;
}

S47GoalLine::FGoals S47GoalLine::Make(const FIn& In)
{
 FGoals G;
 G.Tonight = Clean(In.Next.IsEmpty() ? FString(TEXT("Tonight: nothing is due. Rest, or work the array.")) : TEXT("Tonight: ") + Sentence(FirstSentence(Squash(In.Next), 120)));
 if (In.ReviewDay > 0)
 {
  FString W = FString::Printf(TEXT("This week: the weekly sheet is due on day %d. Costs so far: $%d."), In.ReviewDay, FMath::Max(0, In.SheetSoFar));
  if (In.AllowanceLeft >= 0) W += FString::Printf(TEXT(" $%d of field allowance left."), In.AllowanceLeft);
  G.Week = Clean(W);
 }
 G.Question = Clean(In.CaseTitle.IsEmpty() ? FString(TEXT("The question: none open yet. What you log at OPS may start one."))
  : TEXT("The question: ") + Sentence(OneSentence(In.CaseQuestion)) /* the question is its first sentence only */);
 return G;
}

bool S47GoalLine::SelfTest(TArray<FString>& Lines)
{
 bool OK = true;
 const auto Check = [&](bool B, const TCHAR* Name) { OK &= B; Lines.Add(FString(B ? TEXT("PASS ") : TEXT("FAIL ")) + Name); };
 FIn In; In.Next = TEXT("01:30-04:30  PHOTO LAB: Develop your exposed film at the wet bench."); In.ReviewDay = 8; In.SheetSoFar = 47; In.AllowanceLeft = 35;
 In.CaseTitle = TEXT("THE REPEATED INTERVAL"); In.CaseQuestion = TEXT("Why does the signal repeat every nine minutes? Each test below predicts a different result.");
 const FGoals G = Make(In);
 Check(G.Lines().Num() == 3 && G.Tonight.StartsWith(TEXT("Tonight: ")) && G.Week.StartsWith(TEXT("This week: ")) && G.Question.StartsWith(TEXT("The question: ")), TEXT("N1110-9: three goals, tonight, this week and the question, in that order"));
 Check(G.Week.Contains(TEXT("day 8")) && G.Week.Contains(TEXT("$47")) && G.Week.Contains(TEXT("$35")), TEXT("N1110-9: the week line has the sheet's day, what it comes to and the allowance left"));
 Check(!G.Question.Contains(TEXT("Each test")) && G.Question.EndsWith(TEXT("?")), TEXT("N1110-9: a long question is cut to its first sentence"));
 FIn None;
 const FGoals E = Make(None);
 Check(E.Tonight.Contains(TEXT("nothing is due")) && E.Week.IsEmpty() && E.Question.Contains(TEXT("none open")) && E.Lines().Num() == 2, TEXT("N1110-9: with nothing due and no sheet or case the lines say so and the week line is left off"));
 Check(FirstSentence(TEXT("Dr. Smith called. Then he hung up."), 110) == TEXT("Dr. Smith called. Then he hung up.") && OneSentence(TEXT("Dr. Smith called. Then he hung up.")) == TEXT("Dr. Smith called."), TEXT("N1110-9: Dr. and Mr. do not end a sentence"));
 Check(!G.Tonight.Contains(TEXT("  ")), TEXT("N1110-9: the tonight line has no double blanks"));
 Check(FirstSentence(FString::ChrN(300, TEXT('a')), 110).Len() <= 110, TEXT("N1110-9: a line with no stop is cut to the limit"));
 bool bLint = true;
 for (const FGoals* X : { &G, &E }) for (const FString& L : X->Lines()) bLint &= S47Script::LintText(L).IsEmpty();
 Check(bLint, TEXT("N1110-9: the words keep to the spoiler limits"));
 return OK;
}

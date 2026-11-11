#include "S47Witness.h"
#include "S47Album.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Places.h"
#include "S47RadioTips.h"
#include "S47UI.h"

bool FS47Witness::Add(const FRec& In)
{
	if (In.Id.IsEmpty() || In.Text.IsEmpty() || !FS47Places::Def(In.PlaceId) || !FMath::IsFinite(In.Clock) || Find(In.Id)) return false;
	if (Kept.Num() >= MaxKept)
	{
		const int32 Old = Kept.IndexOfByPredicate([](const FRec& R) { return R.bPaid; });
		if (Old == INDEX_NONE) return false;
		Kept.RemoveAt(Old);
	}
	FRec R = In;
	R.Id = R.Id.Left(32); R.Who = R.Who.Left(48); R.PlaceId = R.PlaceId.Left(64); R.Text = R.Text.Left(MaxText);
	Kept.Add(R);
	return true;
}

bool FS47Witness::IsOfPlace(const FRec& R, double EyeX, double EyeZ, double ForwardX, double ForwardZ, double ShotClock)
{
	const FS47Places::FDef* Def = FS47Places::Def(R.PlaceId);
	if (!Def || !FMath::IsFinite(EyeX) || !FMath::IsFinite(EyeZ) || !FMath::IsFinite(ShotClock)) return false;
	if (ShotClock < R.Clock || ShotClock - R.Clock > Window) return false;
	const double Dx = Def->X - EyeX, Dz = Def->Z - EyeZ, Dist = FMath::Sqrt(Dx * Dx + Dz * Dz);
	if (Dist > Reach) return false;
	if (Dist <= CloseUp) return true;
	const double Len = FMath::Sqrt(ForwardX * ForwardX + ForwardZ * ForwardZ);
	if (!FMath::IsFinite(Len) || Len < 0.05) return false;
	return (Dx * ForwardX + Dz * ForwardZ) / (Dist * Len) >= FMath::Cos(FMath::DegreesToRadians(FacingDegrees));
}

int32 FS47Witness::Match(double EyeX, double EyeZ, double ForwardX, double ForwardZ, double ShotClock) const
{
	for (int32 I = 0; I < Kept.Num(); ++I)
		if (!Kept[I].bPaid && IsOfPlace(Kept[I], EyeX, EyeZ, ForwardX, ForwardZ, ShotClock)) return I;
	return -1;
}

bool FS47Witness::MarkPaid(int32 Index)
{
	if (!Kept.IsValidIndex(Index) || Kept[Index].bPaid) return false;
	Kept[Index].bPaid = true;
	return true;
}

S47J::FObj FS47Witness::Save() const
{
	auto O = S47J::New();
	if (Kept.IsEmpty()) return O;
	TArray<TSharedPtr<FJsonValue>> List;
	for (const FRec& R : Kept)
	{
		auto X = S47J::New();
		S47J::Set(X, TEXT("id"), R.Id); S47J::Set(X, TEXT("who"), R.Who); S47J::Set(X, TEXT("place"), R.PlaceId);
		S47J::Set(X, TEXT("text"), R.Text); S47J::Set(X, TEXT("clock"), R.Clock);
		if (R.bPaid) S47J::Set(X, TEXT("paid"), 1);
		List.Add(MakeShared<FJsonValueObject>(X));
	}
	O->SetArrayField(TEXT("list"), List);
	return O;
}

void FS47Witness::Load(const S47J::FObj& Saved)
{
	// An older save has none: nothing kept, nothing broken.
	Kept.Reset();
	for (const auto& V : S47J::Arr(Saved, TEXT("list")))
	{
		if (!V || V->Type != EJson::Object) continue;
		const auto O = V->AsObject();
		FRec R;
		R.Id = S47J::Str(O, TEXT("id")); R.Who = S47J::Str(O, TEXT("who")); R.PlaceId = S47J::Str(O, TEXT("place"));
		R.Text = S47J::Str(O, TEXT("text")); R.Clock = S47J::Num(O, TEXT("clock")); R.bPaid = S47J::Int(O, TEXT("paid")) != 0;
		const bool bPaid = R.bPaid;
		if (Add(R) && bPaid) Kept.Last().bPaid = true;
	}
}

FString FS47Witness::NoteText(const FRec& R)
{
	return FString::Printf(TEXT("%s says: \"%s\"%s"), *R.Who, *R.Text, R.bPaid ? TEXT(" [x] Matches your photograph.") : TEXT(" [ ] Not confirmed."));
}

int32 FS47Witness::PayPhoto(US47Main* Main, const FS47AlbumShot& Shot)
{
	if (!Main || !Main->Freeform || !Main->Freeform->Tips || !Shot.Pose.IsSet() || !Shot.Pose->Valid()) return -1;
	FS47Witness& W = Main->Freeform->Tips->Witness();
	const FVector& Eye = Shot.Pose->Eye;
	const FVector& Fwd = Shot.Pose->Forward;
	const int32 Index = W.Match(Eye.X, Eye.Z, Fwd.X, Fwd.Z, Shot.Clock);
	if (Index < 0 || !W.MarkPaid(Index)) return -1;
	const FRec R = W.Records()[Index];
	const int32 Paid = Main->Freeform->EarnSide(Bonus, TEXT("witness corroboration"));
	const FS47Places::FDef* Def = FS47Places::Def(R.PlaceId);
	Main->Freeform->RecordEvidence(TEXT("witness-corroboration.") + R.Id,
		FString::Printf(TEXT("OPS CORROBORATION NOTE\nStatement: %s, %s\nPlace: %s\nPhotograph: %s\nThe statement and your own photograph are of the same place. Field allowance: $%d%s.\nPaid once per statement."),
			*R.Who, *FS47Places::SlipClock(R.Clock), Def ? Def->Title : TEXT("the place named"), *Shot.Id, Paid, Paid < Bonus ? TEXT(" (this week's field allowance is used up)") : TEXT("")));
	if (Main->UI) Main->UI->Toast(Paid > 0 ? FString::Printf(TEXT("Your photograph backs up what %s told you: $%d field allowance."), *R.Who, Paid)
		: FString(TEXT("Your photograph backs up a statement. This week's field allowance is used up: no payment.")), 5);
	UE_LOG(LogTemp, Display, TEXT("S47: witness %s corroborated by %s, $%d (N1110)"), *R.Id, *Shot.Id, Paid);
	return Paid;
}

bool FS47Witness::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	const auto Check = [&](bool bCond, const TCHAR* What) { if (!bCond) { bOk = false; Lines.Add(FString(TEXT("FAIL witness: ")) + What); } };
	const FS47Places::FDef* Wind = FS47Places::Def(TEXT("windmill"));
	if (!Wind) { Lines.Add(TEXT("FAIL witness: no windmill place")); return false; }
	FS47Witness W;
	FRec A; A.Id = TEXT("cb01"); A.Who = TEXT("a driver on the CB"); A.PlaceId = TEXT("windmill"); A.Text = TEXT("Saw a light by the windmill."); A.Clock = 20 * 3600.0;
	Check(W.Add(A) && !W.Add(A), TEXT("a statement is kept once"));
	FRec Bad = A; Bad.Id = TEXT("x"); Bad.PlaceId = TEXT("nowhere");
	Check(!W.Add(Bad), TEXT("a statement with no place on the map is not kept"));
	const double X = Wind->X, Z = Wind->Z, Soon = 23 * 3600.0;
	Check(W.Match(X, Z + 100, 0, -1, Soon) == 0, TEXT("a photograph facing the place, the same night, corroborates"));
	Check(W.Match(X, Z + 100, 0, 1, Soon) == -1, TEXT("facing away does not"));
	Check(W.Match(X, Z + 20, 0, 1, Soon) == 0, TEXT("a close-up needs no facing"));
	Check(W.Match(X, Z + 400, 0, -1, Soon) == -1, TEXT("too far does not"));
	Check(W.Match(X, Z + 100, 0, -1, 19 * 3600.0) == -1, TEXT("a photograph from before the statement does not"));
	Check(W.Match(X, Z + 100, 0, -1, A.Clock + 30 * 3600.0) == 0 && W.Match(X, Z + 100, 0, -1, A.Clock + 40 * 3600.0) == -1, TEXT("the next night counts, three nights on does not"));
	Check(W.MarkPaid(0) && !W.MarkPaid(0) && W.Match(X, Z + 100, 0, -1, Soon) == -1, TEXT("the bonus is paid once"));
	FS47Witness Back; Back.Load(S47J::Read(S47J::Write(W.Save())));
	Check(Back.Records().Num() == 1 && Back.Records()[0].bPaid && Back.Records()[0].Text == A.Text && S47J::Write(Back.Save()) == S47J::Write(W.Save()), TEXT("statements and their payment survive Continue"));
	FS47Witness Old; Old.Load(S47J::New());
	Check(Old.Records().IsEmpty() && W.Save()->Values.Num() > 0 && FS47Witness().Save()->Values.Num() == 0, TEXT("an older save has none and an empty set saves nothing"));
	const FString Note = NoteText(Back.Records()[0]);
	Check(Note.Contains(TEXT("driver")) && Note.Contains(TEXT("[x] Matches your photograph")) && !NoteText(FRec()).Contains(TEXT("[x]")) && Bonus == 8 && !Note.Contains(TEXT("taken")), TEXT("the notebook line says who and whether it matched, without explaining"));
	FS47Witness Full;
	for (int32 I = 0; I < MaxKept + 3; ++I) { FRec F = A; F.Id = FString::Printf(TEXT("w%d"), I); Full.Add(F); }
	Check(Full.Records().Num() == MaxKept, TEXT("the notebook keeps a bounded number"));
	return bOk;
}

// story/seen.ts
#include "S47Seen.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"

namespace
{
	// a direction from Unreal's terms to three's (S47Three::DirToUE the other way: the swap is its own inverse)
	FVector DirToThree(const FVector& U) { return FVector(U.X, U.Z, U.Y); }

	FString Hhmm(double C)
	{
		const double D = FMath::Fmod(FMath::Fmod(C, 86400.0) + 86400.0, 86400.0);
		return FString::Printf(TEXT("%02d:%02d"), int32(FMath::FloorToDouble(D / 3600)), int32(FMath::FloorToDouble(FMath::Fmod(D, 3600.0) / 60)));
	}
	FString PadEnd(const FString& S, int32 N) { return S.Len() >= N ? S : S + FString::ChrN(N - S.Len(), TEXT(' ')); }
}

// ---------- the camera ----------

FS47Cam FS47Cam::Of(const AS47Walker* W, double InFar)
{
	FS47Cam C;
	C.Far = InFar;
	if (!W) return C;
	const FRotationMatrix R(W->GetActorRotation());
	C.Pos = S47Three::ToThree(W->EyeUE());
	C.Forward = DirToThree(W->ForwardUE()).GetSafeNormal();
	C.Right = DirToThree(R.GetUnitAxis(EAxis::Y));
	C.Up = DirToThree(R.GetUnitAxis(EAxis::Z));
	C.Fov = W->Fov;
	FVector2D Size(16, 9);
	if (GEngine && GEngine->GameViewport) GEngine->GameViewport->GetViewportSize(Size);
	C.Aspect = Size.X / FMath::Max(1.0, Size.Y);
	return C;
}

FVector FS47Cam::Project(const FVector& P) const
{
	// the view matrix, then three's perspective projection (makePerspective) and the divide by w
	const FVector D = P - Pos;
	const double X = FVector::DotProduct(D, Right), Y = FVector::DotProduct(D, Up), Z = -FVector::DotProduct(D, Forward);
	const double Top = FMath::Tan(FMath::DegreesToRadians(Fov) * 0.5);
	const double Cx = X / (Top * Aspect), Cy = Y / Top, Cz = -(Far + Near) / (Far - Near) * Z - 2 * Far * Near / (Far - Near);
	double W = -Z;
	if (FMath::Abs(W) < 1e-12) W = W < 0 ? -1e-12 : 1e-12;
	return FVector(Cx / W, Cy / W, Cz / W);
}

FQuat FS47Cam::Quaternion() const
{
	// the camera's axes: +X right, +Y up, +Z behind it
	return FQuat(FMatrix(Right, Up, -Forward, FVector::ZeroVector)).GetNormalized();
}

FS47Obj S47SceneObject(AS47Web* Web, const FString& Name)
{
	if (!Web) return FS47Obj();
	for (int32 I = 0; I < Web->PartCount(); ++I)
	{
		const FS47WebNode* N = Web->Node(I);
		if (N && N->Parent < 0 && N->Name == Name) return FS47Obj(Web, I);
	}
	return FS47Obj();
}

bool S47Facing(const FS47Cam& Cam, const FVector& At, double Range, double Deg)
{
	const FVector T = At - Cam.Pos;
	const double D = T.Size();
	if (D > Range) return false;
	if (D < 0.05) return true;
	// f.angleTo(t)
	const double Cos = FMath::Clamp(FVector::DotProduct(Cam.Forward, T) / FMath::Max(1e-12, Cam.Forward.Size() * D), -1.0, 1.0);
	if (FMath::Acos(Cos) > FMath::DegreesToRadians(Deg)) return false;
	const FVector N = Cam.Project(At);
	return N.Z < 1 && FMath::Abs(N.X) <= 1 && FMath::Abs(N.Y) <= 1;
}

// ---------- SeenLog ----------

FS47SeenRef FS47SeenLog::Add(const FS47SeenRef& E, const TOptional<FS47SeenWatch>& W)
{
	Entries.Add(E);
	Added++;
	if (Entries.Num() > MAX) Entries.RemoveAt(0, Entries.Num() - MAX);
	if (W) Watches.Add({ E, *W, 0 });
	return E;
}

FS47SeenRef FS47SeenLog::Last(const FString& Kind, const TOptional<FString>& What) const
{
	for (int32 I = Entries.Num() - 1; I >= 0; --I)
	{
		const FS47SeenRef& E = Entries[I];
		if (E->Kind == Kind && (!What || E->What == *What)) return E;
	}
	return nullptr;
}

bool FS47SeenLog::Watching(const FS47SeenRef& E) const
{
	return Watches.ContainsByPredicate([&](const FWatching& X) { return X.E == E; });
}

void FS47SeenLog::Stop(const FS47SeenRef& E)
{
	Watches.RemoveAll([&](const FWatching& X) { return X.E == E; });
}

void FS47SeenLog::Update(double Dt, const FS47Cam& Cam)
{
	if (!Watches.Num()) return;
	TArray<FWatching> Keep;
	for (FWatching& X : Watches)
	{
		const TOptional<FVector> P = X.W.Pos ? X.W.Pos() : TOptional<FVector>();
		if (X.W.Until ? X.W.Until() : !P) continue;
		if (P && !X.E->Looking.Get(false) && S47Facing(Cam, *P, X.W.Range.Get(300), X.W.Deg.Get(30))) X.E->Looking = true;
		if ((X.T += Dt) < X.W.Secs) Keep.Add(X);
	}
	Watches = MoveTemp(Keep);
}

void FS47SeenLog::Restore(const TArray<TSharedPtr<FJsonValue>>* List)
{
	Watches.Reset();
	Entries.Reset();
	Added = 0;
	// (a save is the game's own, but one that is not what this expects opens with an empty log)
	if (!List) return;
	for (int32 I = FMath::Max(0, List->Num() - MAX); I < List->Num(); ++I)
	{
		const TSharedPtr<FJsonValue>& V = (*List)[I];
		const TSharedPtr<FJsonObject>* O = nullptr;
		if (!V.IsValid() || !V->TryGetObject(O) || !O) continue;
		FString Kind, What;
		double Clock = 0;
		if (!(*O)->TryGetStringField(TEXT("kind"), Kind) || !(*O)->TryGetStringField(TEXT("what"), What) || !(*O)->TryGetNumberField(TEXT("clock"), Clock) || !FMath::IsFinite(Clock)) continue;
		const FS47SeenRef C = MakeShared<FS47SeenEntry>();
		C->Kind = Kind; C->What = What; C->Clock = Clock;
		C->Place = S47J::Str(*O, TEXT("place"));
		C->Area = S47J::Str(*O, TEXT("area"));
		C->Line = S47J::Str(*O, TEXT("line"));
		bool bLooking = false;
		if ((*O)->TryGetBoolField(TEXT("looking"), bLooking)) C->Looking = bLooking;
		C->End = S47J::Str(*O, TEXT("end"));
		const TArray<double> At = S47J::Nums(*O, TEXT("at"));
		if (At.Num() == 2 && S47J::Arr(*O, TEXT("at")).Num() == 2) C->At = FVector2D(At[0], At[1]);
		Entries.Add(C);
	}
	Added = Entries.Num();
}

TArray<TSharedPtr<FJsonValue>> FS47SeenLog::Data() const
{
	TArray<TSharedPtr<FJsonValue>> Out;
	for (const FS47SeenRef& E : Entries)
	{
		const S47J::FObj O = S47J::New();
		S47J::Set(O, TEXT("kind"), E->Kind);
		S47J::Set(O, TEXT("what"), E->What);
		S47J::Set(O, TEXT("clock"), E->Clock);
		S47J::Set(O, TEXT("place"), E->Place);
		S47J::Set(O, TEXT("area"), E->Area);
		S47J::SetOrNull(O, TEXT("line"), E->Line);
		if (E->Looking) S47J::Set(O, TEXT("looking"), *E->Looking); else S47J::SetNull(O, TEXT("looking"));
		if (E->At) S47J::Set(O, TEXT("at"), TArray<double>{ E->At->X, E->At->Y });
		if (!E->End.IsEmpty()) S47J::Set(O, TEXT("end"), E->End);
		Out.Add(MakeShared<FJsonValueObject>(O));
	}
	return Out;
}

FString FS47SeenLog::Text(int32 N) const
{
	const int32 All = Entries.Num();
	TArray<FString> Out = { FString::Printf(TEXT("seen %d%s"), All, All > N ? *FString::Printf(TEXT(", last %d"), N) : TEXT("")) };
	for (int32 I = FMath::Max(0, All - N); I < All; ++I)
	{
		const FS47SeenEntry& E = *Entries[I];
		Out.Add(FString::Printf(TEXT("%s %s %s %s %s%s"), *Hhmm(E.Clock), *PadEnd(E.Kind, 9), *PadEnd(E.What.Left(12), 12), E.Line.IsEmpty() ? TEXT("-") : *E.Line,
			!E.Looking ? TEXT("-") : *E.Looking ? TEXT("looked") : TEXT("missed"), E.End.IsEmpty() ? TEXT("") : *(TEXT(" ") + E.End)));
	}
	return FString::Join(Out, TEXT("\n"));
}

// ---------- self test ----------

bool S47Seen::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s seen: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// a camera at the origin looking down -Z, 70 degrees, 16:9
	FS47Cam Cam;
	Check(S47Facing(Cam, FVector(0, 0, -10)) && !S47Facing(Cam, FVector(0, 0, 10)), TEXT("facing: in front, not behind"));
	Check(!S47Facing(Cam, FVector(0, 0, -400)) && S47Facing(Cam, FVector(0, 0, -400), 500), TEXT("facing: within the range only"));
	Check(S47Facing(Cam, FVector(4, 0, -10), 300, 30) && !S47Facing(Cam, FVector(4, 0, -10), 300, 20), TEXT("facing: within the angle (21.8 degrees off)"));
	Check(S47Facing(Cam, FVector(0, 5, -10), 300, 30) && !S47Facing(Cam, FVector(0, 9, -10), 300, 60), TEXT("facing: on the screen only (42 degrees up is over its top edge at 35)"));
	const FVector N = Cam.Project(FVector(1, 2, -10));
	Check(FMath::Abs(N.X - 0.1 / (FMath::Tan(FMath::DegreesToRadians(35.0)) * 16 / 9)) < 1e-9 && FMath::Abs(N.Y - 0.2 / FMath::Tan(FMath::DegreesToRadians(35.0))) < 1e-9 && N.Z < 1 && Cam.Project(FVector(0, 0, 1)).Z > 1,
		TEXT("project as three's perspective camera"));
	Check(Cam.Quaternion().Equals(FQuat::Identity, 1e-9), TEXT("the camera's quaternion looking down -Z is the identity"));
	// the log: the last of a kind, the cap, a save and back
	FS47SeenLog Log;
	auto Entry = [](const TCHAR* Kind, const TCHAR* What, double Clock)
	{
		const FS47SeenRef E = MakeShared<FS47SeenEntry>();
		E->Kind = Kind; E->What = What; E->Clock = Clock; E->Place = TEXT("Service yard"); E->Area = TEXT("saro"); E->Line = TEXT("B");
		return E;
	};
	Log.Add(Entry(TEXT("glimpse"), TEXT("S-11"), 3 * 3600));
	const FS47SeenRef Call = Log.Add(Entry(TEXT("call"), TEXT("alienVoice"), 3 * 3600 + 60));
	Log.Add(Entry(TEXT("glimpse"), TEXT("S-14"), 3 * 3600 + 120));
	Check(Log.Last(TEXT("glimpse"))->What == TEXT("S-14") && Log.Last(TEXT("glimpse"), FString(TEXT("S-11")))->Clock == 3 * 3600 && !Log.Last(TEXT("face")), TEXT("last of a kind"));
	Call->At = FVector2D(-12, -508);
	Call->Looking = false;
	Call->End = TEXT("away");
	const S47J::FObj Saved = S47J::New();
	Saved->SetArrayField(TEXT("seen"), Log.Data());
	const S47J::FObj Back = S47J::Read(S47J::Write(Saved));
	FS47SeenLog Log2;
	const TArray<TSharedPtr<FJsonValue>> A = S47J::Arr(Back, TEXT("seen"));
	Log2.Restore(&A);
	Check(Log2.Entries.Num() == 3 && Log2.Added == 3 && Log2.Entries[1]->At == FVector2D(-12, -508) && Log2.Entries[1]->Looking == TOptional<bool>(false) && Log2.Entries[1]->End == TEXT("away")
		&& !Log2.Entries[0]->Looking && Log2.Entries[0]->Line == TEXT("B"), TEXT("a save and back"));
	Check(S47J::Write(Back).Contains(TEXT("\"kind\":\"glimpse\",\"what\":\"S-11\",\"clock\":10800,\"place\":\"Service yard\",\"area\":\"saro\",\"line\":\"B\",\"looking\":null}")), TEXT("the saved entry as seen.ts writes it"));
	for (int32 I = 0; I < 310; ++I) Log2.Add(Entry(TEXT("event"), TEXT("steps"), I));
	Check(Log2.Entries.Num() == FS47SeenLog::MAX && Log2.Added == 313 && Log2.Entries[0]->Clock == 10, TEXT("at most 300, the oldest go first"));
	// a watch: looked at once the camera faces it, ended when it has gone
	FS47SeenLog Log3;
	TOptional<FVector> Where = FVector(0, 0, 50);
	FS47SeenWatch W; W.Pos = [&Where] { return Where; }; W.Secs = 60; W.Deg = 25; W.Range = 320;
	const FS47SeenRef G = Log3.Add(Entry(TEXT("glimpse"), TEXT("S-19"), 0), W);
	Log3.Update(0.1, Cam);
	Check(!G->Looking.Get(false) && Log3.Watching(G), TEXT("a watch: not looked at behind the camera"));
	Where = FVector(0, 0, -50);
	Log3.Update(0.1, Cam);
	Check(G->Looking.Get(false), TEXT("a watch: looked at"));
	Where.Reset();
	Log3.Update(0.1, Cam);
	Check(!Log3.Watching(G), TEXT("a watch ends when the thing has gone"));
	Check(Log2.Text(2).StartsWith(TEXT("seen 300, last 2\n00:05 event     steps        B -")), Log2.Text(2));
	return bOk;
}

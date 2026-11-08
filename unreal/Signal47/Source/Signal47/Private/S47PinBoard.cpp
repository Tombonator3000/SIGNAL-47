#include "S47PinBoard.h"
#include "S47Album.h"
#include "S47BoardRules.h"
#include "S47Docs.h"
#include "S47EventPlaces.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47Keys.h"
#include "S47Kit.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47Story.h"
#include "S47Tape.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "SS47FieldMap.h"
#include "Containers/Ticker.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47PinBoard, Log, All);

namespace S47PinBoardLocal
{
	const TCHAR* const PapersId = TEXT("ff:papers");
	FString FindingCard(const FString& Id) { return TEXT("ff:f:") + Id; }
	FString ClipCard(const FString& DocId) { return TEXT("ff:c:") + DocId; }
	FString PrintCard(const FString& ShotId) { return TEXT("ff:p:") + ShotId; }
	FString SectorCard(const FString& Label) { return TEXT("ff:b:") + Label; }
	// A paper's first sentence, for a card: the named thing, not its key.
	FString FirstWords(const FString& Text, int32 Most)
	{
		FString Out = Text;
		int32 Stop = INDEX_NONE;
		if (Out.FindChar(TEXT('\n'), Stop)) Out = Out.Left(Stop);
		Out.TrimStartAndEndInline();
		if (Out.Len() > Most)
		{
			const int32 Space = Out.Left(Most).Find(TEXT(" "), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			Out = Out.Left(Space > Most / 3 ? Space : Most) + TEXT("...");
		}
		return Out;
	}
	// The nearest place with a name, within 250 m: "the windmill on the Baca lease", "the church", "the station".
	FString NearestPlace(const US47Main* M, double X, double Z)
	{
		FString Best;
		double BestD = 250;
		const auto Try = [&Best, &BestD, X, Z](double Px, double Pz, const FString& Named)
		{
			const double Dist = FVector2D::Distance(FVector2D(Px, Pz), FVector2D(X, Z));
			if (Dist < BestD) { BestD = Dist; Best = Named; }
		};
		const FS47Places* Land = M && M->Freeform ? M->Freeform->Places.Get() : nullptr;
		for (const FS47Places::FDef& D : FS47Places::Defs())
		{
			double Px = D.X, Pz = D.Z;
			if (Land) Land->Spot(D.Id, Px, Pz);
			Try(Px, Pz, D.Title);
		}
		if (M && M->Freeform && M->Freeform->EventPlaces)
			for (const FS47EventPlace& Ev : M->Freeform->EventPlaces->All()) if (!Ev.Title.IsEmpty()) Try(Ev.X, Ev.Z, Ev.Title);
		return Best;
	}
}

// ---------- the pure rules ----------

int32 FS47PinBoard::NightOf(double Clock)
{
	return FMath::Max(1, FMath::FloorToInt((Clock - 6 * 3600.0) / 86400.0) + 1);
}

FString FS47PinBoard::StampOf(double Clock)
{
	const double InDay = FMath::Fmod(FMath::Fmod(Clock, 86400.0) + 86400.0, 86400.0);
	return FString::Printf(TEXT("D%d %02d:%02d"), NightOf(Clock), int32(InDay / 3600) % 24, int32(FMath::Fmod(InDay, 3600.0) / 60));
}

FString FS47PinBoard::PlaceNamed(const FString& Text, FString* OutSentence)
{
	for (const FS47Places::FDef& D : FS47Places::Defs())
	{
		FString Title = D.Title;
		if (Title.StartsWith(TEXT("the "))) Title.RightChopInline(4);
		if (Title.Len() < 8) continue; // too short to be a place's own name
		const int32 At = Text.Find(Title, ESearchCase::IgnoreCase);
		if (At == INDEX_NONE) continue;
		if (OutSentence)
		{
			int32 From = At, To = At + Title.Len();
			while (From > 0 && Text[From - 1] != TEXT('.') && Text[From - 1] != TEXT('\n') && Text[From - 1] != TEXT('/')) --From;
			while (To < Text.Len() && Text[To] != TEXT('.') && Text[To] != TEXT('\n')) ++To;
			*OutSentence = Text.Mid(From, FMath::Min(To + 1, Text.Len()) - From).TrimStartAndEnd();
			if (OutSentence->Len() > 160) *OutSentence = OutSentence->Left(157) + TEXT("...");
		}
		return D.Id;
	}
	return FString();
}

TArray<TPair<FString, FString>> FS47PinBoard::Ties(const TArray<FItem>& Things, double SaroX, double SaroZ)
{
	TArray<TPair<FString, FString>> Out;
	TSet<FString> Done;
	const auto Tie = [&Out, &Done](const FString& A, const FString& B)
	{
		if (A == B) return;
		const FString Key = S47Board::Pair(A, B);
		if (Done.Contains(Key)) return;
		Done.Add(Key); Out.Add({ A, B });
	};
	TArray<const FItem*> ByTime;
	for (const FItem& It : Things) if (!It.bBearing) ByTime.Add(&It);
	ByTime.StableSort([](const FItem& A, const FItem& B) { return A.Clock < B.Clock; });
	for (int32 I = 1; I < ByTime.Num(); ++I)
	{
		const FItem& Cur = *ByTime[I];
		if (Cur.bAt)
			for (int32 J = I - 1; J >= 0; --J)
				if (ByTime[J]->bAt && FVector2D::Distance(FVector2D(Cur.X, Cur.Z), FVector2D(ByTime[J]->X, ByTime[J]->Z)) <= 120) { Tie(ByTime[J]->Id, Cur.Id); break; }
		if (Cur.Line >= 0)
			for (int32 J = I - 1; J >= 0; --J)
				if (ByTime[J]->Line == Cur.Line) { Tie(ByTime[J]->Id, Cur.Id); break; }
	}
	for (const FItem& Bearing : Things)
	{
		if (!Bearing.bBearing) continue;
		TArray<TPair<double, const FItem*>> Under;
		for (const FItem* It : ByTime)
		{
			if (!It->bAt || FVector2D::Distance(FVector2D(It->X, It->Z), FVector2D(SaroX, SaroZ)) <= 200) continue;
			const double Off = FMath::Abs(FMath::Fmod(FS47Places::AzimuthOf(SaroX, SaroZ, It->X, It->Z) - Bearing.Az + 540.0, 360.0) - 180.0);
			if (Off <= Bearing.Spread + 1.5) Under.Add({ Off, It });
		}
		Under.StableSort([](const TPair<double, const FItem*>& A, const TPair<double, const FItem*>& B) { return A.Key < B.Key; });
		for (int32 K = 0; K < FMath::Min(3, Under.Num()); ++K) Tie(Bearing.Id, Under[K].Value->Id);
	}
	return Out;
}

// ---------- a game's life ----------

FS47PinBoard::FS47PinBoard(US47Main* InMain) : M(InMain)
{
	using namespace S47PinBoardLocal;
	FS47BookSpec O;
	O.Key = TEXT("ns");
	O.Owns = [](const FString& Pin) { return Pin.StartsWith(TEXT("ff:")) && Pin != PapersId; };
	O.Links = [this]() -> TArray<FString>& { return Links; };
	O.Cards = [this] { return Cards(); };
	O.Auto = [this] { return StationThreads(); };
	O.Free = [](const FString&, const FString&) { return FString(TEXT("Tied with a thread of your own. Tie the same two again to take it down.")); };
	O.Save = [this] { if (M) M->RequestAutosave(); };
	BookPtr = MakeUnique<FS47RuleBook>(MoveTemp(O));
}

// Stop takes the book off the board; at the very end of the game the board goes with it.
FS47PinBoard::~FS47PinBoard()
{
}

void FS47PinBoard::List(bool bOn)
{
	if (!M || !BookPtr || bListed == bOn) return;
	if (bOn) M->Books.AddUnique(BookPtr.Get()); else M->Books.Remove(BookPtr.Get());
	bListed = bOn;
}

void FS47PinBoard::Begin(const S47J::FObj& Saved)
{
	Stop();
	Life = MakeShared<uint8>(0);
	if (Saved.IsValid())
	{
		if (const S47J::FObj Placed = S47J::Obj(Saved, TEXT("where")); Placed.IsValid())
			for (const auto& Entry : Placed->Values)
			{
				const TArray<TSharedPtr<FJsonValue>>* Nums = nullptr;
				if (Where.Num() >= MostKept || !Entry.Value.IsValid() || !Entry.Value->TryGetArray(Nums) || !Nums || Nums->Num() < 3) continue;
				FWhere Wh; Wh.X = (*Nums)[0]->AsNumber(); Wh.Z = (*Nums)[1]->AsNumber(); Wh.Clock = (*Nums)[2]->AsNumber();
				if (FMath::IsFinite(Wh.X) && FMath::IsFinite(Wh.Z) && FMath::IsFinite(Wh.Clock) && FMath::Abs(Wh.X) < 1e6 && FMath::Abs(Wh.Z) < 1e6) Where.Add(FString(*Entry.Key).Left(80), Wh);
			}
		for (const FString& Id : S47J::Strs(Saved, TEXT("up"))) if (Up.Num() < MostKept && Id.StartsWith(TEXT("ff:"))) Up.Add(Id.Left(120));
		for (const FString& Id : S47J::Strs(Saved, TEXT("clips"))) if (Clips.Num() < MostClips && !Id.IsEmpty()) Clips.AddUnique(Id.Left(120));
		for (const FString& K : S47J::Strs(Saved, TEXT("links"))) if (Links.Num() < MostLinks && K.Contains(TEXT("|"))) Links.AddUnique(K.Left(250));
	}
	List(true);
	UE_LOG(LogS47PinBoard, Display, TEXT("S47: pin board: %d on the board, %d placed, %d clippings, %d threads of your own"), Up.Num(), Where.Num(), Clips.Num(), Links.Num());
}

void FS47PinBoard::Stop()
{
	List(false);
	Life.Reset();
	Where.Reset(); Up.Reset(); Clips.Reset(); Links.Reset();
	Look = 0;
}

S47J::FObj FS47PinBoard::Save() const
{
	S47J::FObj O = S47J::New();
	if (!Where.IsEmpty())
	{
		S47J::FObj Placed = S47J::New();
		for (const auto& Entry : Where)
		{
			TArray<TSharedPtr<FJsonValue>> Nums;
			Nums.Add(MakeShared<FJsonValueNumber>(FMath::RoundToDouble(Entry.Value.X * 10) / 10));
			Nums.Add(MakeShared<FJsonValueNumber>(FMath::RoundToDouble(Entry.Value.Z * 10) / 10));
			Nums.Add(MakeShared<FJsonValueNumber>(FMath::RoundToDouble(Entry.Value.Clock)));
			Placed->SetArrayField(Entry.Key, Nums);
		}
		S47J::Set(O, TEXT("where"), Placed);
	}
	if (!Up.IsEmpty()) { TArray<FString> Order = Up.Array(); Order.Sort(); S47J::Set(O, TEXT("up"), Order); }
	if (!Clips.IsEmpty()) S47J::Set(O, TEXT("clips"), Clips);
	if (!Links.IsEmpty()) S47J::Set(O, TEXT("links"), Links);
	return O;
}

bool FS47PinBoard::AtSaro() const
{
	// The station's own ground, as RecordEvidence counts it: not the farm that shares its area.
	return M && M->Web && M->Player && M->Web->Area() == TEXT("saro") && FVector2D(M->Player->Pos.X, M->Player->Pos.Z).Size() < 220;
}

bool FS47PinBoard::SaroAt(double& OutX, double& OutZ) const
{
	return S47OpenLand::RoadOf(TEXT("saro"), 9, 0, OutX, OutZ); // where the bearings are taken from (FS47Places)
}

void FS47PinBoard::Kept(const FString& FindingId)
{
	if (!M || !M->Freeform || FindingId.IsEmpty() || Where.Num() >= MostKept) return;
	FWhere Wh; Wh.Clock = M->Freeform->Clock();
	double Heading = 0; FString Area; bool bDriving = false;
	bool bPlaced = S47FieldMap::Me(M, Wh.X, Wh.Z, Heading, Area, bDriving);
	// A cassette's record is kept on the desk, but what is on it was out where it ran.
	if (FindingId.StartsWith(TEXT("tape_")) && M->Freeform->Tape)
	{
		const int32 TakeNo = FCString::Atoi(*FindingId.RightChop(5));
		for (const FS47Tape::FTake& Tk : M->Freeform->Tape->Takes())
			if (Tk.N == TakeNo && Tk.bPlaced) { Wh.X = Tk.RoadX; Wh.Z = Tk.RoadZ; Wh.Clock = Tk.Clock; bPlaced = true; break; }
	}
	if (!bPlaced) return;
	Where.Add(FindingId, Wh);
	if (AtSaro()) Up.Add(S47PinBoardLocal::FindingCard(FindingId)); // kept here: straight up on the board
}

void FS47PinBoard::Step(double Dt)
{
	if (!M || !M->Freeform || !M->Freeform->Active()) return;
	Look += Dt;
	if (Look < 1.0) return;
	Look = 0;
	if (AtSaro()) PinWhatCameBack(false);
}

void FS47PinBoard::PinWhatCameBack(bool bQuiet)
{
	using namespace S47PinBoardLocal;
	if (!M || !M->Freeform) return;
	int32 Pinned = 0;
	for (const FString& Id : M->Freeform->FindingIdsFrom(0))
	{
		const FString Card = FindingCard(Id);
		if (!Up.Contains(Card)) { Up.Add(Card); ++Pinned; }
	}
	for (const FString& DocId : Clips)
	{
		const FString Card = ClipCard(DocId);
		if (!Up.Contains(Card)) { Up.Add(Card); ++Pinned; }
	}
	if (Pinned <= 0) return;
	UE_LOG(LogS47PinBoard, Display, TEXT("S47: pin board: %d put up on the board at SARO"), Pinned);
	if (!bQuiet && M->UI) M->UI->Toast(FString::Printf(TEXT("You pin what you brought back on the board (%s): %d new."), *S47Keys::Said(TEXT("board")), Pinned), 4.5f);
}

TArray<FS47PinBoard::FItem> FS47PinBoard::Items() const
{
	using namespace S47PinBoardLocal;
	TArray<FItem> Out;
	if (!M || !M->Freeform) return Out;
	const FS47FreeformSession& Fs = *M->Freeform;
	TMap<int32, const FS47Places::FBearing*> FixOfDay;
	if (Fs.Places) for (const FS47Places::FBearing& B : Fs.Places->Bearings()) FixOfDay.Add(B.Day, &B);
	for (const FString& Id : Fs.FindingIdsFrom(0))
	{
		const FString Card = FindingCard(Id);
		if (!Up.Contains(Card)) continue;
		FItem It; It.Id = Card; It.Line = FS47FreeformCases::LineForFinding(Id);
		if (const FWhere* Wh = Where.Find(Id)) { It.X = Wh->X; It.Z = Wh->Z; It.Clock = Wh->Clock; It.bAt = true; }
		else It.Clock = (Fs.FindingDay(Id) - 1) * 86400.0 + 22 * 3600.0;
		if (Id.StartsWith(TEXT("bearing_d")))
			if (const FS47Places::FBearing* const* B = FixOfDay.Find(FCString::Atoi(*Id.RightChop(9)))) { It.bBearing = true; It.bAt = false; It.Az = (*B)->Az; It.Spread = (*B)->Spread; }
		Out.Add(It);
	}
	if (Fs.Places)
		for (const FString& DocId : Clips)
		{
			if (!Up.Contains(ClipCard(DocId))) continue;
			const FS47Doc* D = M->Story ? M->Story->Docs().FindByPredicate([&DocId](const FS47Doc& X) { return X.Id == DocId; }) : nullptr;
			const FString Place = D ? PlaceNamed(D->Page + TEXT("\n") + D->Title) : FString();
			FItem It; It.Id = ClipCard(DocId);
			if (!Place.IsEmpty() && Fs.Places->Spot(Place, It.X, It.Z)) It.bAt = true;
			Out.Add(It);
		}
	if (M->Night && M->Night->Album)
		for (const FS47AlbumShot& Shot : M->Night->Album->Shots())
		{
			if (Shot.bUnavailable || Shot.Url.IsEmpty() || !Shot.IsDeveloped() || !Shot.Pose) continue;
			FItem It; It.Id = PrintCard(Shot.Id); It.Clock = Shot.Clock;
			It.bAt = S47OpenLand::RoadOf(Shot.Pose->Scene, Shot.Pose->Eye.X, Shot.Pose->Eye.Z, It.X, It.Z);
			Out.Add(It);
		}
	if (M->Flow)
	{
		const FS47MapView View = M->Flow->MapView();
		for (const FS47MapSector& Sec : View.Sectors)
		{
			if (Sec.Label.StartsWith(TEXT("D"))) continue; // the array's own bearings are field records already
			FItem It; It.Id = SectorCard(Sec.Label); It.bBearing = true; It.Az = Sec.Az; It.Spread = Sec.Spread;
			Out.Add(It);
		}
	}
	return Out;
}

TArray<TPair<FString, FString>> FS47PinBoard::StationThreads() const
{
	double Sx = 0, Sz = 0;
	if (!SaroAt(Sx, Sz)) return {};
	TArray<FItem> All = Items();
	// Only between what is on the board.
	const TArray<FS47BoardCard> Shown = Cards();
	TSet<FString> Have;
	for (const FS47BoardCard& Shn : Shown) if (Shn.bHave) Have.Add(Shn.Id);
	All.RemoveAll([&Have](const FItem& It) { return !Have.Contains(It.Id); });
	return Ties(All, Sx, Sz);
}

TArray<FS47Doc> FS47PinBoard::Papers() const
{
	TArray<FS47Doc> Out;
	if (!M || !M->Story) return Out;
	for (const FS47Doc& D : M->Story->Docs())
	{
		const FString Finding = D.Id.StartsWith(TEXT("freeform-")) ? D.Id.RightChop(9) : FString();
		if (!Finding.IsEmpty() && M->Freeform && M->Freeform->HasFinding(Finding)) continue; // field records are on the board
		Out.Add(D);
	}
	return Out;
}

TArray<FS47BoardCard> FS47PinBoard::Cards() const
{
	using namespace S47PinBoardLocal;
	TArray<FS47BoardCard> Out;
	if (!M || !M->Freeform) return Out;
	const FS47FreeformSession& Fs = *M->Freeform;
	const FString Still = TEXT("It is in your notebook. It goes up on the board when you are back at SARO.");
	// The field records, newest last; only the latest MostCards stay up, the rest are taken down into the tray's back.
	TArray<FString> Order = Fs.FindingIdsFrom(0);
	Order.StableSort([this, &Fs](const FString& A, const FString& B)
	{
		const FWhere* Wa = Where.Find(A); const FWhere* Wb = Where.Find(B);
		const double Ca = Wa ? Wa->Clock : (Fs.FindingDay(A) - 1) * 86400.0, Cb = Wb ? Wb->Clock : (Fs.FindingDay(B) - 1) * 86400.0;
		return Ca < Cb;
	});
	if (Order.Num() > MostCards) Order.RemoveAt(0, Order.Num() - MostCards);
	TMap<int32, const FS47Places::FBearing*> FixOfDay;
	if (Fs.Places) for (const FS47Places::FBearing& B : Fs.Places->Bearings()) FixOfDay.Add(B.Day, &B);
	for (const FString& Id : Order)
	{
		const FString Text = Fs.FindingText(Id);
		const FWhere* Wh = Where.Find(Id);
		FS47BoardCard Cd;
		Cd.Id = FindingCard(Id); Cd.Doc = TEXT("freeform-") + Id; Cd.Region = TEXT("ns");
		Cd.Kind = Id.StartsWith(TEXT("tape_")) ? TEXT("thing") : TEXT("paper");
		Cd.Stamp = Wh ? StampOf(Wh->Clock) : FString::Printf(TEXT("DAY %d"), Fs.FindingDay(Id));
		Cd.Title = FirstWords(Text, 48);
		if (Id.StartsWith(TEXT("bearing_d")))
			if (const FS47Places::FBearing* const* B = FixOfDay.Find(FCString::Atoi(*Id.RightChop(9))))
			{
				Cd.Title = FString::Printf(TEXT("BEARING %03.0f FROM SARO"), (*B)->Az);
				Cd.Text = FString::Printf(TEXT("Give or take %.0f degrees. Which way, not how far."), (*B)->Spread);
			}
		if (Cd.Text.IsEmpty())
		{
			const FString Place = Wh ? NearestPlace(M, Wh->X, Wh->Z) : FString();
			if (Id.StartsWith(TEXT("tape_"))) Cd.Text = Place.IsEmpty() ? FString(TEXT("A cassette, recorded out on the land.")) : TEXT("A cassette, recorded at ") + Place + TEXT(".");
			else if (!Place.IsEmpty()) Cd.Text = TEXT("At ") + Place + TEXT(".");
			else if (Wh) Cd.Text = TEXT("Out on the land.");
		}
		Cd.Full = Text;
		Cd.bHave = Up.Contains(Cd.Id);
		Cd.Where = Still;
		Out.Add(MoveTemp(Cd));
	}
	// The prints, once the lab has them.
	if (M->Night && M->Night->Album)
		for (const FS47AlbumShot& Shot : M->Night->Album->Shots())
		{
			if (Shot.bUnavailable || Shot.Url.IsEmpty()) continue;
			FS47BoardCard Cd;
			Cd.Id = PrintCard(Shot.Id); Cd.Region = TEXT("ns"); Cd.Kind = TEXT("photo"); Cd.Image = Shot.Url;
			Cd.Stamp = StampOf(Shot.Clock); Cd.Title = Shot.Place.ToUpper(); Cd.Text = Shot.Text;
			Cd.bHave = Shot.IsDeveloped();
			Cd.Where = TEXT("The film is not developed yet. The lab is at SARO.");
			Out.Add(MoveTemp(Cd));
		}
	// The bearings the land itself gave (lights, broadcasts): on the map in pencil, and here.
	if (M->Flow)
	{
		const FS47MapView View = M->Flow->MapView();
		for (const FS47MapSector& Sec : View.Sectors)
		{
			if (Sec.Label.StartsWith(TEXT("D"))) continue;
			FS47BoardCard Cd;
			Cd.Id = SectorCard(Sec.Label); Cd.Region = TEXT("ns"); Cd.Kind = TEXT("paper"); Cd.bHave = true;
			Cd.Stamp = TEXT("BEARING"); Cd.Title = FString::Printf(TEXT("%03.0f FROM SARO"), Sec.Az);
			Cd.Text = FString::Printf(TEXT("Give or take %.0f degrees, night %s. Penciled on the map."), Sec.Spread, *Sec.Label.RightChop(1));
			Out.Add(MoveTemp(Cd));
		}
	}
	// Clippings the player chose from the tray.
	if (M->Story)
		for (const FString& DocId : Clips)
		{
			const FS47Doc* D = M->Story->Docs().FindByPredicate([&DocId](const FS47Doc& X) { return X.Id == DocId; });
			if (!D) continue;
			FString Sentence;
			PlaceNamed(D->Page + TEXT("\n") + D->Title, &Sentence);
			FS47BoardCard Cd;
			Cd.Id = ClipCard(DocId); Cd.Doc = DocId; Cd.Region = TEXT("ns"); Cd.Kind = TEXT("paper");
			Cd.Stamp = TEXT("CLIPPING"); Cd.Title = FirstWords(D->Title, 48); Cd.Text = Sentence; Cd.Full = D->Page;
			Cd.bHave = Up.Contains(Cd.Id);
			Cd.Where = Still;
			Out.Add(MoveTemp(Cd));
		}
	// The tray.
	{
		const int32 InTray = Papers().Num();
		FS47BoardCard Cd;
		Cd.Id = PapersId; Cd.Region = TEXT("ns"); Cd.Kind = TEXT("paper"); Cd.bHave = true;
		Cd.Stamp = TEXT("PAPERS"); Cd.Title = TEXT("THE TRAY");
		Cd.Text = InTray > 0 ? FString::Printf(TEXT("%d papers: the mail, the Ledger, telexes, the night logs."), InTray) : FString(TEXT("Empty."));
		Cd.Action = TEXT("OPEN");
		const TWeakPtr<uint8> Guard = Life;
		FS47PinBoard* Self = const_cast<FS47PinBoard*>(this);
		// After this click: the board's own button must not be torn down under it.
		Cd.OnAction = [Self, Guard]
		{
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self, Guard](float) { if (Guard.IsValid()) Self->OpenTray(); return false; }), 0.f);
		};
		Out.Add(MoveTemp(Cd));
	}
	return Out;
}

bool FS47PinBoard::DevClip(const FString& DocId)
{
	if (!M || !M->Story) return false;
	const FS47Doc* D = M->Story->Docs().FindByPredicate([&DocId](const FS47Doc& X) { return X.Id == DocId; });
	if (!D || PlaceNamed(D->Page + TEXT("\n") + D->Title).IsEmpty()) return false;
	Clips.AddUnique(DocId);
	if (AtSaro()) Up.Add(S47PinBoardLocal::ClipCard(DocId));
	return true;
}

void FS47PinBoard::OpenTray()
{
	using namespace S47PinBoardLocal;
	if (!M || !M->UI) return;
	const TWeakPtr<uint8> Guard = Life;
	TArray<S47Kit::FW> TrayRows;
	TrayRows.Add(S47Kit::Paper(TEXT("The tray by the board: the mail, the Ledger, telexes, the night logs and Dr. Ward's letters. A paper that names a place out on the land can go up on the board as a clipping.")));
	TArray<FS47Doc> Pile = Papers();
	int32 Shown = 0;
	for (int32 I = Pile.Num() - 1; I >= 0 && Shown < 9; --I, ++Shown)
	{
		const FS47Doc D = Pile[I];
		FString Sentence;
		const FString Place = PlaceNamed(D.Page + TEXT("\n") + D.Title, &Sentence);
		const FString Line = !Sentence.IsEmpty() ? Sentence : FirstWords(D.Page.IsEmpty() ? D.Transcript : D.Page, 110);
		TrayRows.Add(S47Kit::Card(D.Title.IsEmpty() ? D.Stamp : D.Title, Line, [this, Guard, D]
		{
			if (!Guard.IsValid() || !M || !M->UI) return;
			M->UI->Document(D, [this, Guard] { if (Guard.IsValid()) FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this, Guard](float) { if (Guard.IsValid()) OpenTray(); return false; }), 0.f); });
		}));
		if (Place.IsEmpty()) continue;
		if (Clips.Contains(D.Id)) { TrayRows.Add(S47Kit::Small(Up.Contains(ClipCard(D.Id)) ? TEXT("On the board.") : TEXT("Goes up on the board when you are back at SARO."))); continue; }
		if (Clips.Num() >= MostClips) continue;
		const FS47Places::FDef* Def = FS47Places::Def(Place);
		TrayRows.Add(S47Kit::Button(FString::Printf(TEXT("Pin it up: %s"), Def ? Def->Title : *Place), [this, Guard, Id = D.Id]
		{
			if (!Guard.IsValid() || !M) return;
			Clips.AddUnique(Id);
			if (AtSaro()) Up.Add(ClipCard(Id));
			else if (M->UI) M->UI->Toast(TEXT("It goes up on the board when you are back at SARO."), 3.5f);
			UE_LOG(LogS47PinBoard, Display, TEXT("S47: pin board: clipping %s chosen from the tray"), *Id);
			M->RequestAutosave();
			OpenTray();
		}));
	}
	if (Pile.Num() > Shown) TrayRows.Add(S47Kit::Small(FString::Printf(TEXT("%d older papers under these, in the notebook."), Pile.Num() - Shown)));
	TrayRows.Add(S47Kit::Button(TEXT("Back to the board"), [this, Guard]
	{
		if (!Guard.IsValid() || !M) return;
		if (M->UI) M->UI->Close();
		if (M->Board.Open) M->Board.Open(FString(), FString());
	}, true));
	M->UI->Open(S47Kit::Shell(M->UI, TEXT("PAPERS"), S47Kit::Col(TrayRows), 900));
}

// ---------- tests ----------

bool FS47PinBoard::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	const auto Check = [&Lines, &bOk](bool bPass, const TCHAR* What)
	{
		Lines.Add(FString::Printf(TEXT("S47: %s pin board: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), What));
		bOk &= bPass;
	};
	Check(NightOf(23 * 3600.0) == 1 && NightOf(29 * 3600.0) == 1 && NightOf(31 * 3600.0) == 2 && NightOf(86400.0 + 23 * 3600.0) == 2, TEXT("a night runs from six in the morning to six the next"));
	Check(StampOf(86400.0 + 23 * 3600.0 + 40 * 60) == TEXT("D2 23:40") && StampOf(2 * 86400.0 + 2 * 3600.0) == TEXT("D2 02:00"), TEXT("the stamp is the night and the clock"));
	FString Sentence;
	Check(PlaceNamed(TEXT("LIGHTS AGAIN\nWhat sat over the windmill on the Baca lease the other night was not a plane. I know a plane."), &Sentence) == TEXT("windmill")
		&& Sentence.StartsWith(TEXT("What sat over")) && Sentence.EndsWith(TEXT("plane.")), TEXT("a paper that names a place, and the sentence that does"));
	Check(PlaceNamed(TEXT("Your bill for October. Thank you.")).IsEmpty(), TEXT("a bill names no place"));
	// Three things at the windmill, one at the tank, a line of inquiry across two of them, a bearing over the tank.
	TArray<FItem> Things;
	const auto Add = [&Things](const TCHAR* Id, double X, double Z, double Clock, int32 Line) { FItem It; It.Id = Id; It.X = X; It.Z = Z; It.Clock = Clock; It.bAt = true; It.Line = Line; Things.Add(It); };
	Add(TEXT("w1"), 170, 720, 100, -1); Add(TEXT("w2"), 200, 760, 200, 1); Add(TEXT("w3"), 150, 700, 300, -1); Add(TEXT("t1"), -190, 1010, 400, 1);
	FItem Bearing; Bearing.Id = TEXT("b"); Bearing.bBearing = true; Bearing.Spread = 4;
	const double Sx = 24, Sz = -300;
	Bearing.Az = FS47Places::AzimuthOf(Sx, Sz, -190, 1010) + 2.0; Things.Add(Bearing);
	const TArray<TPair<FString, FString>> Tied = Ties(Things, Sx, Sz);
	const auto Has = [&Tied](const TCHAR* A, const TCHAR* B) { return Tied.ContainsByPredicate([A, B](const TPair<FString, FString>& Pr) { return (Pr.Key == A && Pr.Value == B) || (Pr.Key == B && Pr.Value == A); }); };
	Check(Has(TEXT("w1"), TEXT("w2")) && Has(TEXT("w2"), TEXT("w3")) && !Has(TEXT("w1"), TEXT("w3")), TEXT("the same place: each to the latest before it, not every pair"));
	Check(Has(TEXT("w2"), TEXT("t1")) && !Has(TEXT("w1"), TEXT("t1")), TEXT("the same line of inquiry: each to the one before it"));
	Check(Has(TEXT("b"), TEXT("t1")) && !Has(TEXT("b"), TEXT("w1")), TEXT("a bearing over the place where something was kept, and not elsewhere"));
	Check(Tied.Num() == 4, TEXT("nothing tied twice and nothing tied from nothing"));
	return bOk;
}

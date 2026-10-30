// story/fieldBook.ts
#include "S47FieldBook.h"

#include "Internationalization/Regex.h"
#include "S47Audio.h"
#include "S47Json.h"
#include "S47UI.h"

const TCHAR* FS47FieldBook::FB_TITLE = TEXT("T. VEGA / FIELD BOOK 3 / JUNE-JULY 47");

namespace
{
	const TCHAR* EVENTS[] = { TEXT("lights"), TEXT("cows"), TEXT("cows2"), TEXT("knocks"), TEXT("lamps"), TEXT("car") };

	// ---------- where tonight keeps each thing ----------
	const TMap<FString, FString>& KEY() { static const TMap<FString, FString> M = { { TEXT("lights"), TEXT("ev:lights") }, { TEXT("cows"), TEXT("ev:cows1") }, { TEXT("cows2"), TEXT("ev:cows") }, { TEXT("knocks"), TEXT("ne:knocks1") }, { TEXT("lamps"), TEXT("ev:lamps") }, { TEXT("car"), TEXT("ev:car") } }; return M; }
	const TMap<FString, FString>& MARK() { static const TMap<FString, FString> M = { { TEXT("lights"), TEXT("lights") }, { TEXT("cows"), TEXT("cows") }, { TEXT("lamps"), TEXT("lamps") }, { TEXT("car"), TEXT("car") } }; return M; }
	/** A few words of the player's own note for each (story/lineLights.ts, pasture.ts, nightEvents.ts, townEvents.ts). */
	const TMap<FString, FString>& NOTE()
	{
		static const TMap<FString, FString> M = { { TEXT("lights"), TEXT("Then they turned off along B and went out") }, { TEXT("cows"), TEXT("Nine head") }, { TEXT("cows2"), TEXT("One missing. A ring of frost") },
			{ TEXT("knocks"), TEXT("Three knocks on the roof of the cab") }, { TEXT("lamps"), TEXT("the street lights out one by one") }, { TEXT("car"), TEXT("The deputy's car on the shoulder") } };
		return M;
	}
	const TMap<FString, FString>& DATE() { static const TMap<FString, FString> M = { { TEXT("lights"), TEXT("July 1") }, { TEXT("cows"), TEXT("July 3") }, { TEXT("cows2"), TEXT("July 4") }, { TEXT("knocks"), TEXT("July 5") }, { TEXT("lamps"), TEXT("July 6") }, { TEXT("car"), TEXT("July 7") } }; return M; }
	/** What he wrote that the player will know again, when it comes after the book (a toast). */
	const TMap<FString, FString>& QUOTE()
	{
		static const TMap<FString, FString> M = { { TEXT("lights"), TEXT("Then they turned off along B and went out.") }, { TEXT("cows"), TEXT("Took a bearing on them: 026. Nine head.") }, { TEXT("cows2"), TEXT("Eight.") },
			{ TEXT("knocks"), TEXT("Three knocks on the roof of the car, slow, from above.") }, { TEXT("lamps"), TEXT("Back on in the same order.") }, { TEXT("car"), TEXT("Nobody in it.") } };
		return M;
	}
	const TMap<FString, FString>& WHAT()
	{
		static const TMap<FString, FString> M = { { TEXT("lights"), TEXT("three lights along B") }, { TEXT("cows"), TEXT("the herd on 026, nine head") }, { TEXT("cows2"), TEXT("the herd, eight, and the ring of frost") },
			{ TEXT("knocks"), TEXT("three knocks on the roof of the car") }, { TEXT("lamps"), TEXT("the lamps on Main Street and the dogs, in the same order") }, { TEXT("car"), TEXT("the county car with nobody in it") } };
		return M;
	}
	const TCHAR* LAST = TEXT("Three on plate 4.\nThere were two of us.");

	// ---------- the board ----------
	const TMap<FString, FS47BookNote>& NOTES()
	{
		static const TMap<FString, FS47BookNote> N = {
			{ TEXT("n.fb.lights"), { TEXT("Three lights along B. He wrote it in July 1947. I saw it tonight.") } },
			{ TEXT("n.fb.cows"), { TEXT("The herd on 026. He wrote it in July 1947. I saw it tonight.") } },
			{ TEXT("n.fb.knocks"), { TEXT("Three knocks on the roof. He wrote it in July 1947. I saw it tonight.") } },
			{ TEXT("n.fb.lamps"), { TEXT("The lamps and the dogs, in the same order. He wrote it in July 1947. I saw it tonight.") } },
			{ TEXT("n.fb.car"), { TEXT("The car on the shoulder, nobody in it. He wrote it in July 1947. I saw it tonight.") } },
			{ TEXT("n.fb.plate"), { TEXT("Plate 4, against the lamp: three by the tripod. He wrote that there were two of them.") } },
		};
		return N;
	}
	TArray<FS47BookRule> RULES()
	{
		TArray<FS47BookRule> R;
		for (const TCHAR* E : { TEXT("lights"), TEXT("cows"), TEXT("knocks"), TEXT("lamps"), TEXT("car") })
		{
			FS47BookRule X; X.A = FString(TEXT("fb.")) + E; X.B = FString(TEXT("tn.")) + E; X.Note = FString(TEXT("n.fb.")) + E; X.Text = NOTES()[X.Note].Text;
			R.Add(X);
		}
		FS47BookRule P; P.A = TEXT("fb.plate"); P.B = TEXT("env4"); P.Note = TEXT("n.fb.plate"); P.Text = NOTES()[TEXT("n.fb.plate")].Text;
		R.Add(P);
		return R;
	}
	struct FTonight { FString Title, Fallback; };
	const TMap<FString, FTonight>& TONIGHT()
	{
		static const TMap<FString, FTonight> M = {
			{ TEXT("lights"), { TEXT("THREE LIGHTS ALONG B"), TEXT("Three lights, low, west of the highway, keeping pace with the truck. Then they turned off along B and went out.") } },
			{ TEXT("cows"), { TEXT("THE HERD ON 026"), TEXT("The whole herd standing facing the same way, on 026. Not one of them moved, with the headlights right on them.") } },
			{ TEXT("knocks"), { TEXT("KNOCKS ON THE ROOF"), TEXT("Driving. Three knocks on the roof of the cab, slow, from above. Nothing on the road behind.") } },
			{ TEXT("lamps"), { TEXT("THE LAMPS IN CALICHE"), TEXT("Main Street, Caliche: the street lights out one by one, south to north, and the dogs barking in the same order. Back on in the same order.") } },
			{ TEXT("car"), { TEXT("THE EMPTY COUNTY CAR"), TEXT("The deputy's car on the shoulder of the highway north of Caliche: engine running, the door open, nobody in it.") } },
		};
		return M;
	}
	// where they lie on chapter three's patch of cork, under the station's records
	const TMap<FString, FVector2D>& AT() { static const TMap<FString, FVector2D> M = { { TEXT("fb"), { 14, 790 } }, { TEXT("tn.lights"), { 370, 790 } }, { TEXT("tn.cows"), { 580, 790 } }, { TEXT("tn.knocks"), { 790, 790 } }, { TEXT("tn.lamps"), { 370, 1020 } }, { TEXT("tn.car"), { 580, 1020 } } }; return M; }
	/** A note as a card's line: the start of it, the whole behind TEXT (as caseBoard.docCard does). */
	FString Short(const FString& T)
	{
		if (T.Len() <= 118) return T;
		const FString S = T.Left(115);
		// .replace(/[\s.,;:]+\S*$/, '')
		FRegexMatcher M(FRegexPattern(TEXT("[\\s.,;:]+\\S*$")), S);
		return (M.FindNext() ? S.Left(M.GetMatchBeginning()) : S) + TEXT("...");
	}

	const TCHAR* ENV_TITLE = TEXT("PLATE 4 / An envelope for T. Vega, 1947");
	const TCHAR* ENV_TEXT = TEXT("An envelope addressed to T. Vega, care of Sierra Motor Court. Postmarked July 1947, never opened.\n\nA photo shop's envelope from Roswell, sealed with wax. In pencil along the bottom, in the shop's hand: Plate 4. 1 print. 35c.");
	const TCHAR* ENV_LIT = TEXT("Held up in front of the lamp on the counter, the paper goes the colour of weak tea. The flaps show through, and a print inside, a little smaller than the envelope.\n\nOn the print, small and dark against something lighter: a tripod with the transit on it, and three people standing by it. Two close together. The third a step off, and not as sharp.");

	FString TimeIn(const TArray<TPair<FString, FString>>& Times, const FString& E) { const TPair<FString, FString>* P = Times.FindByPredicate([&E](const TPair<FString, FString>& X) { return X.Key == E; }); return P ? P->Value : FString(); }
}

// ---------- FbSave ----------

TSharedPtr<FJsonValue> FS47FbSave::ToJson() const
{
	const S47J::FObj O = S47J::New();
	S47J::Set(O, TEXT("at"), At);
	const S47J::FObj T = S47J::New();
	for (const TPair<FString, FString>& P : Times) S47J::Set(T, *P.Key, P.Value);
	S47J::Set(O, TEXT("times"), T);
	S47J::Set(O, TEXT("had"), Had);
	S47J::Set(O, TEXT("told"), Told);
	S47J::Set(O, TEXT("links"), Links);
	if (Page) S47J::Set(O, TEXT("page"), *Page);
	return MakeShared<FJsonValueObject>(O);
}

TOptional<FS47FbSave> FS47FbSave::FromJson(const TSharedPtr<FJsonValue>& V)
{
	const TSharedPtr<FJsonObject>* O = nullptr;
	if (!V.IsValid() || !V->TryGetObject(O) || !O || !O->IsValid()) return {};
	FS47FbSave S;
	S.At = S47J::Num(*O, TEXT("at"));
	if (const S47J::FObj T = S47J::Obj(*O, TEXT("times")))
	{
		// (in the book's own order)
		for (const TCHAR* E : EVENTS) { FString Tm; if (T->TryGetStringField(E, Tm)) S.Times.Add({ E, Tm }); }
	}
	S.Had = S47J::Strs(*O, TEXT("had"));
	S.Told = S47J::Strs(*O, TEXT("told"));
	S.Links = S47J::Strs(*O, TEXT("links"));
	if (S47J::Has(*O, TEXT("page"))) S.Page = S47J::Int(*O, TEXT("page"));
	return S;
}

// ---------- the book ----------

TArray<S47FieldBookView::FBookPage> FS47FieldBook::FbPages(const TArray<TPair<FString, FString>>& Times)
{
	auto T = [&Times](const TCHAR* E) { const FString V = TimeIn(Times, E); return V.IsEmpty() ? FString() : V + TEXT(". "); };
	const TArray<FString> Pages = {
		TEXT("T. VEGA\nFIELD BOOK 3\nJUNE - JULY 47\n\nSTA 01.  A, B, C.\n\nIf found, Sierra Motor Court,\nCaliche, N. Mex."),
		TEXT("Jun 9. Footing for A poured on the transit line. Backsight to B. N. on the rod.\n\nJun 14. C staked out east, forty stakes. Cable laid from the hut to the pier.\n\nJun 20. N.'s first plate has a mark on it that should not be there. Told her it was a mistake.\n\nJun 27. Night sight out on C. Lost an hour. One side of my face burned. Told N. it was the lamp."),
		FString::Printf(TEXT("Jul 1. %sThe highway north of the motel. Three lights, low, west of the road, keeping pace with the car. Then they turned off along B and went out.\n\nN. says ranch trucks. Ranch trucks have two lights."), *T(TEXT("lights"))),
		FString::Printf(TEXT("Jul 3. %sThe pasture south of the motel. The whole herd standing the same way. Took a bearing on them: 026. Nine head. Not one of them moved, with the headlights right on them.\n\nJul 4. %sEight. Where the ninth stood the grass is flat in a ring, white with frost. In July."), *T(TEXT("cows")), *T(TEXT("cows2"))),
		FString::Printf(TEXT("Jul 5. %sComing back up from Caliche, N. asleep against the door. Three knocks on the roof of the car, slow, from above. Nothing on the road behind.\n\nI did not stop. I did not wake her."), *T(TEXT("knocks"))),
		FString::Printf(TEXT("Jul 6. %sCaliche. The lamps on Main Street out one by one, south to north, and the dogs barking in the same order. Back on in the same order.\n\nAs if something had walked up the street."), *T(TEXT("lamps"))),
		FString::Printf(TEXT("Jul 7. %sThe county car on the shoulder north of Caliche. Engine running, the door open, his hat on the seat. Nobody in it.\n\nWe waited. Nobody came back for it."), *T(TEXT("car"))),
		TEXT("Jul --. Plate 4, off the camera on the pier. Held it up to the lamp in the hut.\n\nSent it in to Roswell for a print, care of the motel.\n\nN. will not look at it."),
		LAST,
	};
	// the transcription: the same words, the lines run together within a paragraph (not on the cover
	// and the last page, which are set as they stand)
	TArray<S47FieldBookView::FBookPage> Out;
	for (int32 i = 0; i < Pages.Num(); ++i)
	{
		const FString& P = Pages[i];
		FString Typed = P;
		if (i != 0 && i != Pages.Num() - 1)
		{
			// .replace(/(?<!\n)\n(?!\n)/g, ' ')
			Typed.Reset();
			for (int32 c = 0; c < P.Len(); ++c)
			{
				const bool bSingle = P[c] == TEXT('\n') && (c == 0 || P[c - 1] != TEXT('\n')) && (c + 1 >= P.Len() || P[c + 1] != TEXT('\n'));
				Typed.AppendChar(bSingle ? TEXT(' ') : P[c]);
			}
		}
		Out.Add({ P, Typed, i == Pages.Num() - 1 });
	}
	return Out;
}

FS47FieldBook::FS47FieldBook(FS47FieldBookDeps InD) : D(MoveTemp(InD))
{
	FS47BookSpec O;
	O.Key = TEXT("ch3");
	O.Owns = [](const FString& N) { return N.StartsWith(TEXT("fb.")) || N.StartsWith(TEXT("tn.")); };
	O.Notes = NOTES(); O.Rules = RULES();
	O.Links = [this]() -> TArray<FString>& { FS47FbSave* Sv = S(); if (Sv) return Sv->Links; NoLinks.Reset(); return NoLinks; };
	O.Wrong = [](const FString&, const FS47BookRule&) { return TOptional<FS47Result>(); };
	O.Save = [this] { Write(); if (D.Save) D.Save(); };
	O.Cards = [this] { return Cards(); };
	O.Empty = TEXT("Not the same thing. Find his line for what you saw.");
	Book = MakeUnique<FS47RuleBook>(MoveTemp(O));
}

FS47FbSave* FS47FieldBook::S() const
{
	const TSharedPtr<FJsonValue> J = D.State.Get ? D.State.Get() : nullptr;
	if (J != CachedJson) { CachedJson = J; Cached = FS47FbSave::FromJson(J); }
	return Cached ? &*Cached : nullptr;
}

void FS47FieldBook::Write()
{
	// (the book's state is chapter three's own JSON: what changes here is written back to it)
	if (!Cached) return;
	CachedJson = Cached->ToJson();
	if (D.State.Set) D.State.Set(CachedJson);
}

// ---------- what tonight has ----------

bool FS47FieldBook::Has(const FString& E) const { const FString* K = KEY().Find(E); return D.Seen && K && D.Seen->Contains(*K); }

FString FS47FieldBook::NoteOf(const FString& E) const
{
	const TArray<FString> N = D.Notes ? D.Notes() : TArray<FString>();
	const FString& Words = NOTE()[E];
	for (int32 i = N.Num() - 1; i >= 0; --i) if (N[i].Contains(Words)) return N[i];
	return FString();
}

FString FS47FieldBook::TimeOf(const FString& E) const
{
	// the minute the player had it: the map's mark, or the note's clock
	if (const FString* M = MARK().Find(E); M && D.Seen)
	{
		const FString Pre = FString::Printf(TEXT("mark:%s@"), **M);
		for (const FString& K : D.Seen->Keys) if (K.StartsWith(Pre)) return S47Text::ClockText(FCString::Atod(*K.Mid(K.Find(TEXT("@")) + 1)), false);
	}
	const FString N = NoteOf(E);
	if (N.Len() >= 6 && FChar::IsDigit(N[0]) && FChar::IsDigit(N[1]) && N[2] == TEXT(':') && FChar::IsDigit(N[3]) && FChar::IsDigit(N[4]) && N[5] == TEXT('.')) return N.Left(5);
	return FString();
}

// ---------- the drawer and the book ----------

FString FS47FieldBook::DrawerLabel() const { return Read() || Draft ? TEXT("T. Vega's field book") : TEXT("Open the drawer"); }

FString FS47FieldBook::DrawerExamine() const
{
	return Read() || Draft ? TEXT("Pencils, a dry rubber band, and a field book in green cloth: T. VEGA, FIELD BOOK 3. Inside the cover: Tomás Vega, field engineer. If found, return to STATION 01.")
		: TEXT("A shallow drawer under the bench top, below the lamp. The pull is worn bright.");
}

void FS47FieldBook::UseDrawer()
{
	if (bOpening) return;
	if (Read() || Draft) { const FS47FbSave* Sv = S(); OpenBook(Sv && Sv->Page ? *Sv->Page : 0); return; }
	bOpening = true;
	if (D.Drawer) D.Drawer(true, false);
	if (US47Audio* A = D.Audio.Get())
	{
		FS47Sound L; L.Gain = 0.22f; L.Rate = 0.7f; A->Sfx(A->Or(TEXT("door_latch"), TEXT("switch")), L);
		FS47Sound T; T.Gain = 0.18f; T.Rate = 1.7f; T.When = 0.25f; A->Sfx(TEXT("thudSoft"), T);
	}
	if (D.Toast) D.Toast(TEXT("The drawer sticks, then comes. Two pencils, a dry rubber band, and a field book in green cloth."), 3.6f);
	if (D.After) D.After(1.1, [this] { OpenBook(0); });
}

void FS47FieldBook::OpenBook(int32 Start)
{
	bOpening = false;
	if (Start < 0) { const FS47FbSave* Sv = S(); Start = Sv && Sv->Page ? *Sv->Page : 0; }
	TArray<TPair<FString, FString>> Times;
	if (const FS47FbSave* Sv = S()) Times = Sv->Times;
	else { if (!Draft) Draft = TimesNow(); Times = *Draft; }
	const TArray<S47FieldBookView::FBookPage> Pages = FbPages(Times);
	TSharedRef<bool> bEnd = MakeShared<bool>(false);
	S47FieldBookView::FBookOpts O;
	O.Title = TEXT("T. Vega / Field book 3"); O.Pages = Pages; O.Start = Start;
	const int32 N = Pages.Num();
	O.OnPage = [this, bEnd, N](int32 Page)
	{
		if (FS47FbSave* Sv = S()) { Sv->Page = Page; Write(); }
		if (Page == N - 1) *bEnd = true;
	};
	O.OnClose = [this, bEnd, Times](int32 Last)
	{
		if (FS47FbSave* Sv = S()) { Sv->Page = Last; Write(); if (D.Save) D.Save(); }
		else if (*bEnd) FirstRead(Times, Last);
	};
	S47FieldBookView::Book(D.UI.Get(), D.Audio.Get(), O);
}

TArray<TPair<FString, FString>> FS47FieldBook::TimesNow() const
{
	TArray<TPair<FString, FString>> Out;
	for (const TCHAR* E : EVENTS) if (Has(E)) { const FString T = TimeOf(E); if (!T.IsEmpty()) Out.Add({ E, T }); }
	return Out;
}

void FS47FieldBook::FirstRead(const TArray<TPair<FString, FString>>& Times, int32 Last)
{
	FS47FbSave Sv;
	Sv.At = D.Clock ? D.Clock() : 0; Sv.Times = Times; Sv.Page = Last;
	for (const TCHAR* E : EVENTS) if (Has(E)) Sv.Had.Add(E);
	Cached = Sv;
	Write();
	Draft.Reset();
	if (D.File) D.File(Doc());
	TArray<FString> SeenNow;
	for (const TCHAR* E : EVENTS) if (Has(E)) SeenNow.Add(E);
	TArray<FString> List;
	for (const FString& E : SeenNow)
	{
		if (E == TEXT("cows2") && SeenNow.Contains(TEXT("cows"))) continue;
		List.Add(E == TEXT("cows") && SeenNow.Contains(TEXT("cows2")) ? FString(TEXT("the herd on 026, nine and then eight")) : WHAT()[E]);
	}
	auto And = [](const TArray<FString>& L) { if (L.Num() <= 1) return FString::Join(L, TEXT("")); TArray<FString> Head = L; Head.Pop(); return FString::Join(Head, TEXT(", ")) + TEXT(" and ") + L.Last(); };
	bool bMore = false;
	for (const TCHAR* E : EVENTS) bMore |= !Has(E);
	const double Clock = D.Clock ? D.Clock() : 0;
	FString N = FString::Printf(TEXT("%s. The hut at STATION 01, the drawer under the bench: T. Vega's field book, June and July 1947."), *S47Text::ClockText(Clock, false));
	if (List.Num()) N += FString::Printf(TEXT(" In July he wrote down %s. I saw %s tonight, and the times he has are mine."), *And(List), List.Num() > 1 ? TEXT("them") : TEXT("it"));
	if (bMore) N += List.Num() ? TEXT(" There is more in it that I have not seen. Not yet.") : TEXT(" I have not seen any of it. Not yet.");
	N += TEXT(" The last page: \"Three on plate 4. There were two of us.\"");
	if (D.Seen && D.Seen->Contains(TEXT("env:lamp"))) N += TEXT(" The envelope in the motel office has Plate 4 on it.");
	if (D.Note) D.Note(N);
	if (D.Toast) D.Toast(List.Num() ? TEXT("In July 1947, he wrote down tonight.") : TEXT("July 1947. None of it has happened tonight. Not yet."), 4);
	if (D.Foretell) D.Foretell();
	if (D.Refresh) D.Refresh();
	if (D.Save) D.Save();
}

FS47Doc FS47FieldBook::Doc() const
{
	const FS47FbSave* Sv = S();
	const TArray<S47FieldBookView::FBookPage> Pages = FbPages(Sv ? Sv->Times : TArray<TPair<FString, FString>>());
	TArray<FString> Hand;
	for (const S47FieldBookView::FBookPage& P : Pages) Hand.Add(P.Hand);
	FS47Doc Dc;
	Dc.Id = TEXT("fieldbook"); Dc.Title = TEXT("FIELD BOOK 3 / T. Vega, June and July 1947"); Dc.Kind = TEXT("hand"); Dc.Stamp = TEXT("T. VEGA");
	Dc.Page = FString::Join(Hand, TEXT("\n\n"));
	Dc.Transcript = TEXT("From the drawer under the bench in the hut at STATION 01. Pencil, in a field book bound in green cloth. The survey in June; in July, other things.");
	return Dc;
}

// ---------- the envelope in the motel office ----------

FString FS47FieldBook::EnvelopeLabel() const { return D.Seen && D.Seen->Contains(TEXT("env:lamp")) ? TEXT("Envelope, plate 4") : TEXT("Sealed envelope"); }

S47FieldBookView::FEnvelopeView FS47FieldBook::UseEnvelope()
{
	if (FrontArt.IsEmpty()) FrontArt = S47FieldBookView::EnvelopeFront();
	if (LitArt.IsEmpty()) LitArt = S47FieldBookView::EnvelopeLit();
	S47FieldBookView::FEnvelopeOpts O;
	O.Title = ENV_TITLE; O.Front = FrontArt; O.Lit = LitArt; O.Text = ENV_TEXT; O.LitText = EnvLit();
	O.OnHold = [this] { Held(); };
	O.OnClose = [] {};
	return S47FieldBookView::Envelope(D.UI.Get(), D.Audio.Get(), O);
}

FString FS47FieldBook::EnvLit() const { return Read() ? FString(ENV_LIT) + TEXT("\n\nThere were two of them. He wrote it on the last page of his field book.") : FString(ENV_LIT); }

void FS47FieldBook::Held()
{
	if (!D.Seen || D.Seen->Contains(TEXT("env:lamp"))) return;
	D.Seen->Add(TEXT("env:lamp"));
	if (D.File) D.File(EnvelopeDoc());
	if (D.Note) D.Note(FString::Printf(TEXT("%s. Sierra Motor Court, the office. On the counter, an envelope for T. Vega that nobody has opened since 1947: \"Plate 4. 1 print. 35c.\" Held up to the lamp: a print inside, and on it a tripod and three small figures standing by it.%s"),
		*S47Text::ClockText(D.Clock ? D.Clock() : 0, false), Read() ? TEXT(" He wrote that there were two of them.") : TEXT("")));
	if (D.Refresh) D.Refresh();
	if (D.Save) D.Save();
}

FS47Doc FS47FieldBook::EnvelopeDoc()
{
	if (LitArt.IsEmpty()) LitArt = S47FieldBookView::EnvelopeLit();
	FS47Doc Dc;
	Dc.Id = TEXT("env4"); Dc.Title = ENV_TITLE; Dc.Kind = TEXT("typed"); Dc.Stamp = TEXT("PLATE 4");
	Dc.Page = TEXT("PRINTS . ENLARGEMENTS . FINISHING\nROSWELL, N. MEX.\n\nT. Vega\nc/o Sierra Motor Court\nCaliche, N. Mex.\n\n~ Plate 4. 1 print. 35c.");
	Dc.Image = LitArt;
	Dc.Transcript = FString(ENV_LIT) + TEXT("\n\nThe envelope is still sealed. It stays on the counter.");
	return Dc;
}

// ---------- the board's cards ----------

TArray<FS47BoardCard> FS47FieldBook::Cards() const
{
	const FS47FbSave* Sv = S();
	if (!Sv) return {};
	auto Place = [](FS47BoardCard& C, const FString& K) { const FVector2D P = AT()[K]; C.X = P.X; C.Y = P.Y; };
	auto T = [Sv](const TCHAR* E) { const FString V = Sv->Time(E); return V.IsEmpty() ? FString() : TEXT(", ") + V; };
	FS47BoardCard BookCard;
	BookCard.Id = TEXT("fb"); BookCard.Doc = TEXT("fieldbook"); BookCard.Stamp = TEXT("FIELD BOOK 3"); BookCard.Title = TEXT("T. VEGA, JUNE AND JULY 1947"); BookCard.bHave = true; BookCard.W = 320;
	Place(BookCard, TEXT("fb"));
	TArray<FString> Typed;
	for (const S47FieldBookView::FBookPage& P : FbPages(Sv->Times)) Typed.Add(P.Typed);
	BookCard.Full = FString::Join(Typed, TEXT("\n\n"));
	BookCard.Lines = {
		{ TEXT("fb.lights"), FString::Printf(TEXT("Jul 1%s. Three lights, low, west of the road. They turned off along B and went out."), *T(TEXT("lights"))), FString() },
		{ TEXT("fb.cows"), FString::Printf(TEXT("Jul 3%s. The herd all one way, on 026. Nine head. Jul 4%s. Eight."), *T(TEXT("cows")), *T(TEXT("cows2"))), FString() },
		{ TEXT("fb.knocks"), FString::Printf(TEXT("Jul 5%s. Three knocks on the roof of the car, slow, from above."), *T(TEXT("knocks"))), FString() },
		{ TEXT("fb.lamps"), FString::Printf(TEXT("Jul 6%s. The lamps on Main Street out one by one, the dogs with them."), *T(TEXT("lamps"))), FString() },
		{ TEXT("fb.car"), FString::Printf(TEXT("Jul 7%s. The county car on the shoulder. Nobody in it."), *T(TEXT("car"))), FString() },
		{ TEXT("fb.plate"), TEXT("Three on plate 4. There were two of us."), FString() },
	};
	TArray<FS47BoardCard> Out = { BookCard };
	for (const TCHAR* K : { TEXT("lights"), TEXT("cows"), TEXT("knocks"), TEXT("lamps"), TEXT("car") })
	{
		const FString Ks = K;
		const bool bHave = Ks == TEXT("cows") ? Has(TEXT("cows")) || Has(TEXT("cows2")) : Has(Ks);
		const FString Note = Ks == TEXT("cows") ? (Has(TEXT("cows2")) ? NoteOf(TEXT("cows2")) : NoteOf(TEXT("cows"))) : NoteOf(Ks);
		const FString Text = !Note.IsEmpty() ? Note : TONIGHT()[Ks].Fallback;
		FS47BoardCard C;
		C.Id = TEXT("tn.") + Ks; C.Stamp = TEXT("TONIGHT"); C.Title = TONIGHT()[Ks].Title; C.Kind = TEXT("thing"); C.bHave = bHave; C.W = 190;
		Place(C, C.Id);
		C.Where = TEXT("In his book. Not yet in mine."); C.Text = Short(Text); C.ReadingText = Text;
		if (Text.Len() > 118) C.Full = Text;
		Out.Add(C);
	}
	return Out;
}

// ---------- after the book ----------

void FS47FieldBook::Update(double Dt, bool bOk, bool bDriving, TFunctionRef<bool()> Knock)
{
	FS47FbSave* Sv = S();
	if (!Sv) { DriveT = 0; return; }
	for (const TCHAR* Ec : EVENTS)
	{
		const FString E = Ec;
		if (Sv->Had.Contains(E) || !Sv->Time(E).IsEmpty() || Sv->Told.Contains(E) || !Has(E)) continue;
		// its own note and toast first; then this one, with nothing open over the game
		const double* P = Pending.Find(E);
		const double Left = (P ? *P : (E == TEXT("cows2") ? 4.5 : 3.0)) - Dt;
		const US47UI* UI = D.UI.Get();
		if (Left > 0 || (UI && UI->Modal())) { Pending.Add(E, FMath::Max(0.0, Left)); continue; }
		Pending.Remove(E);
		Sv->Told.Add(E);
		Write();
		if (D.Note) D.Note(FString::Printf(TEXT("%s. It is in T. Vega's field book, under %s, 1947: %s. He wrote it in July 1947. I saw it tonight."), *S47Text::ClockText(D.Clock ? D.Clock() : 0, false), *DATE()[E], *WHAT()[E]));
		if (D.Toast) D.Toast(FString::Printf(TEXT("T. Vega, %s, 1947: \"%s\""), *DATE()[E], *QUOTE()[E]), 4.6f);
		if (D.Refresh) D.Refresh();
		if (D.Save) D.Save();
	}
	// the knocks: some way into a drive, once nothing else is going on, and has not been for a while
	if (Has(TEXT("knocks")) || bKnocked) return;
	Calm = bOk ? Calm + Dt : 0;
	if (!bDriving) { DriveT = 0; return; }
	if (!bOk) return;
	DriveT += Dt;
	if (DriveT > KNOCK_AFTER && Calm > KNOCK_QUIET && Knock()) bKnocked = true;
}

// ---------- a case opened ----------

void FS47FieldBook::Restore()
{
	Pending.Reset(); DriveT = 0; Calm = 0; bKnocked = false; Draft.Reset(); bOpening = false;
	if (Read() && D.File) D.File(Doc());
	if (D.Seen && D.Seen->Contains(TEXT("env:lamp")) && D.File) D.File(EnvelopeDoc());
	if (D.Drawer) D.Drawer(Read(), true);
	if (Read() && D.Foretell) D.Foretell();
}

void FS47FieldBook::DrawerNow() { if (D.Drawer) D.Drawer(Read() || Draft.IsSet(), true); }

bool FS47FieldBook::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s field book: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	// the pages, with the minutes the player had
	const TArray<S47FieldBookView::FBookPage> P = FbPages({ { TEXT("lights"), TEXT("02:41") } });
	Check(P.Num() == 9 && P[2].Hand.StartsWith(TEXT("Jul 1. 02:41. The highway north of the motel.")) && P[8].bLast && P[8].Typed == LAST, TEXT("nine pages, the minute he has is mine"));
	Check(P[1].Typed.Contains(TEXT("the rod.\n\nJun 14")) && !P[0].Typed.Contains(TEXT("BOOK 3 JUNE")) && P[2].Typed.EndsWith(TEXT("N. says ranch trucks. Ranch trucks have two lights.")), TEXT("the transcription runs the lines of a paragraph together"));
	Check(Short(FString::ChrN(100, TEXT('a')) + TEXT(" wordlong ") + FString::ChrN(20, TEXT('b'))) == FString::ChrN(100, TEXT('a')) + TEXT(" wordlong..."), TEXT("a card's short line"));
	// read once: the book keeps what it had, and what comes after it is written down
	FS47TownSeen Seen;
	Seen.Add(TEXT("ev:lights")); Seen.Add(TEXT("mark:lights@9660"));
	TSharedPtr<FJsonValue> State;
	TArray<FString> Notes;
	FS47FieldBookDeps Dp;
	Dp.Seen = &Seen;
	Dp.State.Get = [&State] { return State; };
	Dp.State.Set = [&State](const TSharedPtr<FJsonValue>& V) { State = V; };
	Dp.Note = [&Notes](const FString& N) { Notes.Add(N); };
	Dp.Notes = [&Notes] { return Notes; };
	Dp.Clock = [] { return 4.0 * 3600; };
	FS47FieldBook B(Dp);
	Check(!B.Read() && B.DrawerLabel() == TEXT("Open the drawer"), TEXT("not read yet"));
	B.FirstRead(B.TimesNow(), 8);
	const TOptional<FS47FbSave> Sv = FS47FbSave::FromJson(State);
	Check(B.Read() && Sv && Sv->Time(TEXT("lights")) == TEXT("02:41") && Sv->Had == TArray<FString>{ TEXT("lights") } && Sv->Page == 8, TEXT("read: the minute of the lights, kept in chapter three's state"));
	Check(Notes.Num() == 1 && Notes[0].Contains(TEXT("In July he wrote down three lights along B. I saw it tonight")) && Notes[0].Contains(TEXT("There is more in it that I have not seen. Not yet.")), TEXT("the note says what was seen"));
	Seen.Add(TEXT("ev:car"));
	for (int32 i = 0; i < 4; ++i) B.Update(1, false, false, [] { return false; });
	const TOptional<FS47FbSave> Sv2 = FS47FbSave::FromJson(State);
	Check(Sv2 && Sv2->Told.Contains(TEXT("car")) && Notes.Last().Contains(TEXT("under July 7, 1947: the county car with nobody in it")), TEXT("the car, seen after the book: under July 7"));
	const TArray<FS47BoardCard> C = B.Cards();
	Check(C.Num() == 6 && C[0].Lines[0].Text == TEXT("Jul 1, 02:41. Three lights, low, west of the road. They turned off along B and went out.") && C[5].bHave && !C[2].bHave, TEXT("the book's card and tonight's"));
	return bOk;
}

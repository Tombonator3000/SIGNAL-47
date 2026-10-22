// story/lostNights.ts: the nights SARO printed a negative range
#include "S47LostNights.h"

#include "S47Seen.h"

namespace
{
	// ---------- what the town has (story/caliche.ts) ----------
	namespace K
	{
		const TCHAR* Pins = TEXT("ask:deputy:green"); const TCHAR* Old = TEXT("ask:attendant:old"); const TCHAR* Tow = TEXT("ask:attendant:tow");
		const TCHAR* Wife = TEXT("ask:radio_woman:husband"); const TCHAR* Tape = TEXT("ask:host:tape"); const TCHAR* Boy = TEXT("ev:boy");
		const TCHAR* Watch = TEXT("ln:watch"); const TCHAR* Reel = TEXT("ln:reel"); const TCHAR* WatchAsked = TEXT("ln:watch:asked"); const TCHAR* TowShown = TEXT("ln:tow:shown");
	}
	FS47LostLine Aside(const FString& Text) { return FS47LostLine(FString(), Text); }
	FS47LostLine You(const FString& Text) { return FS47LostLine(TEXT("YOU"), Text); }
	FS47LostLine Earl(const FString& Text) { return FS47LostLine(TEXT("EARL"), Text); }
	FS47LostLine Driver(const FString& Text) { return FS47LostLine(TEXT("THE DRIVER"), Text); }
	const TCHAR* REEL = TEXT("REEL, 9/22/81");

	// ---------- the board ----------
	/** Night Shift's patch of cork has the evening's papers along the top; the binder, the green pins,
	 *  what the three in Caliche said and the question lie under them. */
	struct FAt { double X, Y, W; };
	const TMap<FString, FAt>& AT() { static const TMap<FString, FAt> M = { { TEXT("ln.binder"), { 14, 372, 282 } }, { TEXT("ln.pins"), { 308, 372, 282 } }, { TEXT("ln.said"), { 602, 372, 290 } }, { TEXT("ln.q39"), { 904, 372, 262 } } }; return M; }
	void At(FS47BoardCard& C, const TCHAR* Id) { const FAt& A = AT()[Id]; C.X = A.X; C.Y = A.Y; C.W = A.W; }
	// the October 1985 night: the hours (the binder, the green pin, the attendant's own words, the
	// ticket's times) and the minute (the binder, the ticket's remarks, the watch)
	const TArray<FString> HOURS = { TEXT("ln.b85"), TEXT("ln.g85"), TEXT("ln.w85"), TEXT("ln.th") }, MINUTE = { TEXT("ln.b85"), TEXT("ln.tq"), TEXT("ln.watch") };
	TArray<FString> N85() { TArray<FString> O; for (const FString& S : HOURS) O.AddUnique(S); for (const FString& S : MINUTE) O.AddUnique(S); return O; }
	const TArray<FString> N83 = { TEXT("ln.b83"), TEXT("ln.g83"), TEXT("ln.w83") }, N81 = { TEXT("ln.b81"), TEXT("ln.g81"), TEXT("ln.w81"), TEXT("ln.reel") };
	/** Two pins on the same card (the board says so itself). */
	const TMap<FString, FString>& CARD()
	{
		static const TMap<FString, FString> M = { { TEXT("ln.b81"), TEXT("b") }, { TEXT("ln.b83"), TEXT("b") }, { TEXT("ln.b85"), TEXT("b") }, { TEXT("ln.g81"), TEXT("g") }, { TEXT("ln.g83"), TEXT("g") }, { TEXT("ln.g85"), TEXT("g") },
			{ TEXT("ln.w81"), TEXT("w") }, { TEXT("ln.w83"), TEXT("w") }, { TEXT("ln.w85"), TEXT("w") }, { TEXT("ln.tq"), TEXT("t") }, { TEXT("ln.th"), TEXT("t") } };
		return M;
	}
	/** The notes that tie a night of the binder's to Caliche (the others: two things in Caliche). */
	const TMap<FString, FString>& NIGHT() { static const TMap<FString, FString> M = { { TEXT("n85"), TEXT("85") }, { TEXT("n85m"), TEXT("85") }, { TEXT("n83"), TEXT("83") }, { TEXT("n81"), TEXT("81") } }; return M; }
	const TCHAR* Q39 = TEXT("-39 tonight. Who loses time tonight?");
	/** What gives one of the three nights a date in Caliche (story/caliche.ts). */
	const TArray<const TCHAR*> DATES = { K::Pins, K::Old, K::Wife, K::Tape };
	FString Clock(double S) { return S47Text::ClockText(S, false); }
	/** .replace(/^~ /gm, '') */
	FString NoPencil(const FString& S)
	{
		TArray<FString> L = S47Text::Split(S, TEXT("\n"));
		for (FString& X : L) if (X.StartsWith(TEXT("~ "))) X = X.Mid(2);
		return FString::Join(L, TEXT("\n"));
	}
}

FS47Doc FS47LostNights::TowSlip()
{
	FS47Doc D;
	D.Id = TEXT("towslip"); D.Title = TEXT("Tow ticket / Caliche Service, No. 0419"); D.Kind = TEXT("typed"); D.Stamp = TEXT("TOW");
	D.Page = TEXT("CALICHE SERVICE\nGAS . DIESEL . TOWING\nCALICHE, N. MEX.          No. 0419\n\nDATE\n~ 10-20-85\nCALLED IN               BY\n~ 1:40 AM               ---\nOUT\n~ 1:45 AM\nVEHICLE\n~ Tractor & reefer trailer\nFROM\n~ Old Roswell Rd, past 8 mi post\nTO\n~ Mesa Diner lot\nREMARKS\n~ Driver in cab. Says engine quit 11:58.\n~ His watch says so too.\nIN\n~ 4:25 AM\nHOURS\n~ 2.7\n\n~ 40 min job. 16 mi. Where's the other 2 hrs?");
	D.Transcript = TEXT("A yellow carbon off the spike of tow tickets by the register at the gas station, filled in in ballpoint. A tractor and a refrigerated trailer, towed in off the old road from just past the eight-mile post, to the diner's lot. Out at a quarter to two, in at twenty-five past four: 2.7 hours, for a forty minute job.\n\nThe line for who called it in is blank. In the remarks: the driver was in the cab, and said his engine quit at 11:58. His watch said so too.\n\nUnder it, in the attendant's pencil, a sum that does not come out.");
	return D;
}

FS47Doc FS47LostNights::ReelPaper(bool bHeard)
{
	const FString Page = FString(TEXT("1420 AM / THE REEL FROM 9/22/81\n\nPencil on the box, in Earl's hand:\n9/22/81. OFF AIR.\n\nON THE REEL, a boy, over a telephone line:\n\"Hello? Is this the radio? There's a star with a tail on it, real low in the south. A little tail.\"\nThen nothing. Not hiss: nothing.\n\"I'm calling from my dad's place, out on the ranch. There's a light over the mesa too. It isn't moving.\"\nNothing again, shorter.\n\"He's out with the men. It's hot out. It's July.\"\nA click on the line.\n\n"))
		+ (bHeard ? TEXT("Word for word what I heard the boy say on the Night Line tonight.") : TEXT("Earl says it is word for word what the boy said on the Night Line tonight."))
		+ TEXT(" Where it goes quiet is where Earl answered him tonight, and for as long.\n\nEARL:\n\"I answered him tonight.\"\n\"He wasn't answering me.\"\n\"Station was off that night in eighty-one. There was nobody on the air to answer him.\"\n\"He left room for me anyway.\"");
	FS47Doc D;
	D.Id = TEXT("witness-reel"); D.Title = TEXT("Witness / Earl's reel from 9/22/81"); D.Kind = TEXT("hand"); D.Stamp = TEXT("REYES");
	D.Page = TEXT("REYES / NOTEBOOK COPY\n\n") + Page;
	D.Transcript = TEXT("Copied into my notebook at the studio door. What is on the reel, and what Earl said.\n\n") + Page;
	return D;
}

bool FS47LostNights::Has(const FString& Key) const { return D.Seen && D.Seen->Contains(Key); }

FS47LostNights::FS47LostNights(FS47LostNightsDeps InD) : D(MoveTemp(InD))
{
	auto Note = [this](const TCHAR* Id, const TCHAR* Text, TFunction<FString()> Fn = nullptr) { FS47BookNote N; N.Text = Text; N.TextFn = MoveTemp(Fn); NotesT.Add(Id, N); };
	Note(TEXT("n85"), TEXT("The night SARO printed -38, the attendant lost two hours on the old road."));
	Note(TEXT("n85m"), TEXT("SARO printed -38 at 23:58:31. On the old road a rig quit at 11:58, and the driver's watch stopped with it."));
	Note(TEXT("n85x"), TEXT("October 19, 1985: a rig quit on the old road at two minutes to midnight, and the man who towed it in lost two hours."));
	Note(TEXT("n85h"), TEXT("The attendant's two hours last October: out at a quarter to two, back at twenty-five past four, for a forty minute job."));
	Note(TEXT("n85w"), TEXT("11:58 on the ticket and on his watch. It stopped with the engine, and it never went again."));
	Note(TEXT("n83h"), TEXT("The deputy's pin and the woman on the bench: her husband, March 1983. Forty-seven minutes."));
	// (what Earl did with the reel changes once he has played it to the player: the notes say so)
	Note(TEXT("n81h"), TEXT(""), [this] { return Has(K::Reel)
		? FString(TEXT("The same boy, the same September: the call Earl kept on reel. He played it for me tonight."))
		: FString(TEXT("The same boy, the same September: the call Earl kept on reel and never played back.")); });
	Note(TEXT("n83"), TEXT(""), [this] { return Has(K::Wife)
		? FString(TEXT("The night SARO printed -36, a man in Caliche went out to his cattle and was gone forty-seven minutes."))
		: FString(TEXT("The night SARO printed -36, a man in Caliche lost track of the time.")); });
	// (with Earl's reel known and not yet heard, the note says where that night is kept: the way back to
	// the studio door)
	Note(TEXT("n81"), TEXT(""), [this] { return !Has(K::Tape)
		? FString(TEXT("The night SARO printed -34, a boy called the radio in Caliche."))
		: Has(K::Reel) ? FString(TEXT("The night SARO printed -34, a boy called the radio in Caliche, and the station was off the air."))
		: FString(TEXT("The night SARO printed -34, a boy called the radio in Caliche, and the station was off the air. Earl still has that call on reel. The binder has the minute: 01:52.")); });
	Note(TEXT("nreel"), TEXT("The boy on the Night Line tonight is on Earl's reel from September 1981, word for word. Earl answered a recording."));

	// the rules
	TArray<FS47BookRule> Rules;
	auto Tie = [this, &Rules](const FString& A, const FString& B, const FString& N)
	{
		FS47BookRule R; R.A = A; R.B = B; R.Note = N; R.Text = NoteText(N);
		R.TextFn = [this, N] { return Say(N); };
		Rules.Add(R);
	};
	auto Each = [](const TArray<FString>& List, TFunctionRef<void(const FString&, const FString&)> F)
	{
		for (int32 i = 0; i < List.Num(); ++i) for (int32 j = i + 1; j < List.Num(); ++j)
		{
			const FString& A = List[i]; const FString& B = List[j];
			const FString* Ca = CARD().Find(A); const FString* Cb = CARD().Find(B);
			if (!Ca || !Cb || *Ca != *Cb) F(A, B);
		}
	};
	// with the binder, SARO's night; without it, only that two things in Caliche are the same night
	Each(N85(), [&](const FString& A, const FString& B)
	{
		const FString Other = A == TEXT("ln.b85") ? B : B == TEXT("ln.b85") ? A : FString();
		Tie(A, B, !Other.IsEmpty() ? (HOURS.Contains(Other) ? TEXT("n85") : TEXT("n85m"))
			: HOURS.Contains(A) && HOURS.Contains(B) ? TEXT("n85h") : MINUTE.Contains(A) && MINUTE.Contains(B) ? TEXT("n85w") : TEXT("n85x"));
	});
	Each(N83, [&](const FString& A, const FString& B) { Tie(A, B, A == TEXT("ln.b83") || B == TEXT("ln.b83") ? TEXT("n83") : TEXT("n83h")); });
	Each(N81, [&](const FString& A, const FString& B) { Tie(A, B, A == TEXT("ln.b81") || B == TEXT("ln.b81") ? TEXT("n81") : TEXT("n81h")); });
	Tie(TEXT("ln.reel"), TEXT("ln.boy"), TEXT("nreel"));
	// tonight's boy and the boy of 1981, before the reel has been heard: the thread holds, thin
	for (const TCHAR* B : { TEXT("ln.g81"), TEXT("ln.w81") })
	{
		FS47BookRule R; R.A = TEXT("ln.boy"); R.B = B;
		R.TextFn = [this] { return Has(K::Reel)
			? FString(TEXT("A boy on the radio, tonight and in September 1981. Earl played me the one from 1981: the same words."))
			: FString(TEXT("A boy on the radio, tonight and in September 1981. Earl kept the one from 1981. He has never played it.")); };
		R.Text = R.TextFn();
		Rules.Add(R);
	}

	FS47BookQuestion Q;
	Q.Id = TEXT("ln.q39"); Q.Stamp = TEXT("-39"); Q.Text = Q39; Q.Slots = 0;
	Q.Hints = { TEXT("Nothing on the board answers it."), TEXT("Every night SARO printed a negative range, somebody in Caliche lost time. Tonight it printed -39."), TEXT("Keep an eye on the clock tonight.") };
	Q.Open = [this] { return Trio(); };
	Q.Done = [] { return FString(); };
	Q.ElsewhereFn = [this] { return Answer(); };
	Q.X = AT()[TEXT("ln.q39")].X; Q.Y = AT()[TEXT("ln.q39")].Y; Q.W = AT()[TEXT("ln.q39")].W;

	FS47BookSpec O;
	O.Key = TEXT("ns");
	O.Owns = [](const FString& N) { return N.StartsWith(TEXT("ln.")); };
	O.Notes = NotesT;
	O.Rules = Rules;
	O.Questions = { Q };
	O.Links = [this]() -> TArray<FString>& { return D.Links(); };
	O.Wrong = [](const FString&, const FS47BookRule&) { return TOptional<FS47Result>(); };
	O.Save = [this] { Sync(); if (D.Save) D.Save(); };
	O.Cards = [this] { return Cards(); };
	O.Empty = TEXT("Not the same night. Look at the dates.");
	Book = MakeUnique<FS47RuleBook>(MoveTemp(O));
}

FString FS47LostNights::NoteText(const FString& Id) const { const FS47BookNote* N = NotesT.Find(Id); return N ? N->Now() : FString(); }

TArray<FS47RuleBook*> FS47LostNights::Books()
{
	bool bDate = false;
	for (const TCHAR* K : DATES) bDate |= Has(K);
	return D.Open && D.Open() && ((D.Binder && D.Binder()) || bDate) ? TArray<FS47RuleBook*>{ Book.Get() } : TArray<FS47RuleBook*>();
}

FString FS47LostNights::Say(const FString& Note)
{
	const FString T = NoteText(Note);
	if (!bJustAsked) return T;
	bJustAsked = false;
	return FString::Printf(TEXT("%s And a question with it: %s"), *T, Q39);
}

TSet<FString> FS47LostNights::Nights() const
{
	// which of the three nights are tied on the board
	TSet<FString> Out;
	if (!D.Links) return Out;
	for (const FString& Lk : D.Links())
	{
		const FS47BookRule* R = Book ? Book->RuleOf(Lk) : nullptr;
		if (const FString* N = R && !R->Note.IsEmpty() ? NIGHT().Find(R->Note) : nullptr) Out.Add(*N);
	}
	return Out;
}

bool FS47LostNights::Trio() const { const TSet<FString> N = Nights(); return N.Contains(TEXT("85")) && N.Contains(TEXT("83")) && N.Contains(TEXT("81")); }

void FS47LostNights::Sync()
{
	// after each thread: what it found goes with the town's part of the case (Earl needs the date), and
	// with the three nights tied, the question and a line in the notebook, once
	if (!D.Seen) return;
	for (const FString& N : Nights()) D.Seen->Add(TEXT("ln:") + N);
	if (Trio() && !D.Seen->Contains(TEXT("ln:q39")))
	{
		D.Seen->Add(TEXT("ln:q39"));
		bJustAsked = true;
		if (D.Note) D.Note(FString::Printf(TEXT("%s. The case board. Three nights SARO printed a negative range, somebody in Caliche lost time: September 1981, a boy on the radio; March 1983, a man out with his cattle; October 1985, the attendant on the old road. Tonight SARO printed -39."), *Clock(D.Clock ? D.Clock() : 0)));
	}
}

FString FS47LostNights::Answer() const
{
	// under the question, once the player has been away: the minutes, in their own hand
	TArray<FString> Away;
	const TArray<FString> Notes = D.Notes ? D.Notes() : TArray<FString>();
	for (const FString& N : Notes)
	{
		// /^(\d\d:\d\d) to (\d\d:\d\d)\. Nothing\.$/
		auto Hm = [&N](int32 I) { return FChar::IsDigit(N[I]) && FChar::IsDigit(N[I + 1]) && N[I + 2] == TEXT(':') && FChar::IsDigit(N[I + 3]) && FChar::IsDigit(N[I + 4]); };
		if (N.Len() == 24 && Hm(0) && N.Mid(5, 4) == TEXT(" to ") && Hm(9) && N.Mid(14) == TEXT(". Nothing.")) Away.Add(N.Left(5) + TEXT(" to ") + N.Mid(9, 5));
	}
	if (!Away.Num()) return TEXT("Nobody has an answer for it. Not yet.");
	const FString List = Away.Num() == 1 ? Away[0] : FString::Join(TArray<FString>(Away.GetData(), Away.Num() - 1), TEXT(", ")) + TEXT(" and ") + Away.Last();
	return FString::Printf(TEXT("Under it, in my own hand:\n~ %s\nI do not remember writing it."), *List);
}

TArray<FS47BoardCard> FS47LostNights::Cards() const
{
	TArray<FS47BoardCard> Out;
	auto Line = [](const TCHAR* Id, const FString& Text) { FS47BoardLine L; L.Id = Id; L.Text = Text; return L; };
	{
		FS47BoardCard C;
		C.Id = TEXT("ln.binder"); C.Stamp = TEXT("EXCEPTIONS"); C.Title = TEXT("SOLVER EXCEPTIONS BINDER"); At(C, TEXT("ln.binder"));
		if (!(D.Binder && D.Binder())) { C.bHave = false; C.Where = TEXT("On the shelf over the printer in the control room. The old printouts in it have dates."); }
		else
		{
			const FS47Doc B = S47Docs::Binder();
			C.Doc = TEXT("binder"); C.bHave = true;
			C.Text = TEXT("Three printouts from before tonight, each stamped INPUT ERROR, and an older sheet.");
			C.Full = NoPencil(B.Page) + TEXT("\n\n") + B.Transcript;
			C.Lines = { Line(TEXT("ln.b81"), TEXT("09/22/81  01:52:07  -34 LY. In pencil: clock?")),
				Line(TEXT("ln.b83"), TEXT("03/02/83  03:10:44  -36 LY. In pencil: baseline file reloaded.")),
				Line(TEXT("ln.b85"), TEXT("10/19/85  23:58:31  -38 LY. In pencil: third time. Ask Systems.")) };
		}
		Out.Add(C);
	}
	{
		FS47BoardCard C;
		C.Id = TEXT("ln.pins"); C.Stamp = TEXT("GREEN PINS"); C.Title = TEXT("THE DEPUTY'S MAP, CALICHE"); C.Kind = TEXT("thing"); At(C, TEXT("ln.pins"));
		if (Has(K::Pins))
		{
			C.bHave = true;
			C.Text = TEXT("\"Green is people who lost track of the time.\" Three of them in town.");
			C.Lines = { Line(TEXT("ln.g81"), TEXT("A kid who called the radio station in September of '81. Family moved away after.")),
				Line(TEXT("ln.g83"), TEXT("A man whose wife sits out at the laundromat nights. March of '83.")),
				Line(TEXT("ln.g85"), TEXT("The fella at the gas station. October nineteenth, last year. Went out to tow a rig and lost two hours.")) };
			Out.Add(C);
		}
		else if (Has(TEXT("pins")) || Has(TEXT("met:deputy"))) { C.bHave = false; C.Where = TEXT("Green pins on the deputy's map. He knows what each one is."); Out.Add(C); }
	}
	// what the three said themselves, a line each as they said it
	TArray<FS47BoardLine> Said;
	if (Has(K::Tape)) Said.Add(Line(TEXT("ln.w81"), TEXT("Earl, 1420 AM: \"September of '81, a boy called in. I've never played it back. The station was off.\"")));
	if (Has(K::Wife)) Said.Add(Line(TEXT("ln.w83"), TEXT("The woman on the bench: \"He went out to look at the cattle one night. March, three years ago. Forty-seven minutes.\"")));
	if (Has(K::Old)) Said.Add(Line(TEXT("ln.w85"), TEXT("The attendant: \"Towed a rig in off it once. Last October. Left here a quarter to two. Got back twenty-five past four.\"")));
	if (Said.Num())
	{
		FS47BoardCard C;
		C.Id = TEXT("ln.said"); C.Stamp = TEXT("CALICHE"); C.Title = TEXT("WHAT THEY TOLD ME"); C.Kind = TEXT("thing"); C.bHave = true; At(C, TEXT("ln.said"));
		C.Text = TEXT("In their own words, from my notebook."); C.Lines = Said;
		Out.Add(C);
	}
	if (Has(K::Tow))
	{
		const FS47Doc T = TowSlip();
		FS47BoardCard C;
		C.Id = TEXT("ln.tow"); C.Doc = TEXT("towslip"); C.Stamp = TEXT("TOW"); C.Title = TEXT("CALICHE SERVICE, No. 0419"); C.bHave = true; C.W = 262;
		C.Text = TEXT("10-20-85. A tractor and reefer, off the old road past the 8 mi post. Called in by: blank.");
		C.Full = NoPencil(T.Page) + TEXT("\n\n") + T.Transcript;
		C.Lines = { Line(TEXT("ln.tq"), TEXT("Driver in cab. Says engine quit 11:58. His watch says so too.")), Line(TEXT("ln.th"), TEXT("Out 1:45 AM. In 4:25 AM. Hours 2.7. In pencil: 40 min job.")) };
		Out.Add(C);
	}
	if (Has(K::Watch))
	{
		FS47BoardCard C;
		C.Id = TEXT("ln.watch"); C.Stamp = TEXT("MESA DINER"); C.Title = TEXT("THE DRIVER'S WATCH"); C.Kind = TEXT("thing"); C.bHave = true; C.W = 220;
		C.Text = Has(K::WatchAsked) ? TEXT("Two minutes to twelve. It stopped the minute his rig quit on the old road last October, and it will not go.")
			: TEXT("Two minutes to twelve, on the wrist of the driver at the counter. It is not running.");
		Out.Add(C);
	}
	if (Has(K::Reel))
	{
		FS47BoardCard C;
		C.Id = TEXT("ln.reel"); C.Doc = TEXT("witness-reel"); C.Stamp = TEXT("REEL"); C.Title = TEXT("EARL'S REEL, 9/22/81"); C.Kind = TEXT("thing"); C.bHave = true; C.W = 240;
		C.Text = TEXT("The boy, word for word as tonight. Where Earl answered him tonight, nothing on the tape. Not even hiss.");
		Out.Add(C);
	}
	if (Has(K::Boy))
	{
		const TArray<FString> Notes = D.Notes ? D.Notes() : TArray<FString>();
		const FString* N = Notes.FindByPredicate([](const FString& X) { return X.Contains(TEXT("A boy calling about a star with a tail")); });
		FS47BoardCard C;
		C.Id = TEXT("ln.boy"); C.Stamp = TEXT("TONIGHT"); C.Title = TEXT("THE BOY ON THE NIGHT LINE"); C.Kind = TEXT("thing"); C.bHave = true; C.W = 230;
		C.Text = N ? *N : FString(TEXT("Night Line, 1420. A boy calling about a star with a tail, low in the south. He said it was July."));
		Out.Add(C);
	}
	else if (Has(K::Reel))
	{
		FS47BoardCard C;
		C.Id = TEXT("ln.boy"); C.Stamp = TEXT("TONIGHT"); C.Title = TEXT("THE BOY ON THE NIGHT LINE"); C.Kind = TEXT("thing"); C.bHave = false; C.W = 230;
		C.Where = TEXT("Earl had him on the Night Line tonight. I did not hear it.");
		Out.Add(C);
	}
	return Out;
}

// ---------- Earl's reel ----------

bool FS47LostNights::ReelReady() const { return Has(TEXT("ln:81")) && !Has(K::Reel) && !ReelBusy() && (!D.Quiet || D.Quiet()); }

void FS47LostNights::PlayReel(TFunction<void(const TArray<FS47LostLine>&)> Lines, TFunction<void(bool)> Done)
{
	if (D.HushVoices) D.HushVoices();
	if (!D.Tape.Play) { if (Done) Done(false); return; }
	TSharedRef<TArray<FS47LostLine>> Page = MakeShared<TArray<FS47LostLine>>();
	auto Put = [Page, Lines](const FS47LostLine& L) { Page->Add(L); if (Lines) Lines(*Page); };
	D.Tape.Play([Page, Put](const FS47LostTape::FCue& C)
	{
		if (C.Kind == TEXT("start")) { Page->Reset(); Put(Aside(TEXT("Hiss from the monitor. An open telephone line, a long way off."))); }
		else if (C.Kind == TEXT("boy")) Put(FS47LostLine(REEL, C.Text));
		else if (C.Kind == TEXT("hole")) Put(Aside(C.I == 0 ? TEXT("Then nothing. Not hiss: nothing at all, and the reels going round.") : TEXT("Nothing again, shorter. Earl's lips are moving.")));
		else if (C.Kind == TEXT("click")) Put(Aside(TEXT("The hiss again, and a click on the line, as if a receiver were put down at the other end.")));
	},
	[this, Lines, Done](bool bComplete)
	{
		if (!bComplete) { if (Done) Done(false); return; }
		const bool bHeard = Has(K::Boy);
		Played(bHeard);
		TArray<FS47LostLine> L = { Aside(TEXT("He stops the deck. He does not rewind it.")) };
		if (bHeard) L.Add(Aside(TEXT("Word for word what you heard on the Night Line tonight."))); else L.Add(Earl(TEXT("That's what he said tonight. On the air. Every word.")));
		L.Add(Earl(TEXT("Where it goes quiet, that's me. That's how long I talked to him tonight. To the second.")));
		L.Add(Earl(TEXT("I answered him tonight.")));
		L.Add(Earl(TEXT("He wasn't answering me.")));
		L.Add(Earl(TEXT("Station was off that night in eighty-one. There was nobody on the air to answer him.")));
		L.Add(Earl(TEXT("He left room for me anyway.")));
		L.Add(Aside(TEXT("He puts the reel back in its box. On the spine, in pencil, in his own hand: 9/22/81. OFF AIR.")));
		if (Lines) Lines(L);
		if (Done) Done(true);
	});
}

void FS47LostNights::Played(bool bHeard)
{
	if (!D.Seen) return;
	D.Seen->Add(K::Reel);
	if (D.BoyCalled) D.BoyCalled();
	if (D.File) D.File(ReelPaper(bHeard));
	if (D.Note) D.Note(FString::Printf(TEXT("%s. 1420 AM. Earl played me the reel he never played: 9/22/81, the night SARO printed -34, with the station off the air. The boy on it says what %s on the Night Line tonight, word for word. Twice the tape has nothing on it, not even hiss, for as long as Earl answered him tonight. Earl: \"I answered him tonight. He wasn't answering me.\""),
		*Clock(D.Clock ? D.Clock() : 0), bHeard ? TEXT("I heard the boy say") : TEXT("Earl says the boy said")));
	if (D.Refresh) D.Refresh();
	if (D.Save) D.Save();
}

// ---------- the driver at the counter in the Mesa Diner (Chapter5.ts) ----------

void FS47LostNights::SeeWatch()
{
	if (!D.Seen || Has(K::Watch)) return;
	D.Seen->Add(K::Watch);
	if (D.Note) D.Note(FString::Printf(TEXT("%s. Mesa Diner. The driver at the counter: his wristwatch says two minutes to twelve. It is not running."), *Clock(D.Clock ? D.Clock() : 0)));
	if (D.Refresh) D.Refresh();
	if (D.Save) D.Save();
}

FString FS47LostNights::DriverExamine()
{
	SeeWatch();
	return TEXT("A driver over two eggs and a thermos, his cap pushed back. His wristwatch says two minutes to twelve.");
}

TArray<FS47LostLine> FS47LostNights::DriverOpen()
{
	if (Has(K::Watch)) return {};
	SeeWatch();
	return { Aside(TEXT("He checks his watch, the way people do. It says two minutes to twelve.")) };
}

TArray<FS47LostNights::FChoice> FS47LostNights::DriverChoices(bool bOctober) const
{
	TArray<FChoice> Out;
	if (Has(K::Watch) && !Has(K::WatchAsked)) Out.Add({ TEXT("ln:watch"), TEXT("Ask about his watch") });
	if (Has(K::Tow) && bOctober && !Has(K::TowShown)) Out.Add({ TEXT("ln:tow"), TEXT("Show him the tow ticket from the gas station") });
	return Out;
}

TOptional<TArray<FS47LostLine>> FS47LostNights::DriverChoose(const FString& Id)
{
	const FString T = Clock(D.Clock ? D.Clock() : 0);
	if (Id == TEXT("ln:watch"))
	{
		if (D.Seen) D.Seen->Add(K::WatchAsked);
		if (D.Note) D.Note(FString::Printf(TEXT("%s. The driver's watch has said 11:58 since last October. It stopped the minute his rig quit on the old road, and it will not go again. A man in Roswell found nothing wrong with it. He still wears it."), *T));
		if (D.Refresh) D.Refresh();
		if (D.Save) D.Save();
		return TArray<FS47LostLine>{
			You(TEXT("Your watch has stopped.")),
			Driver(TEXT("Since October.")),
			Aside(TEXT("He turns his wrist and looks at it anyway, the way you look at a watch that works.")),
			Driver(TEXT("Quit out there the same minute she did. Two minutes to midnight.")),
			Driver(TEXT("Wound it, shook it, took it to a man in Roswell. Nothing wrong with it, he says. It just doesn't go.")),
			Driver(TEXT("So I wear it.")),
		};
	}
	if (Id == TEXT("ln:tow"))
	{
		if (D.Seen) D.Seen->Add(K::TowShown);
		if (D.Note) D.Note(FString::Printf(TEXT("%s. Mesa Diner. I showed the driver the tow ticket: his rig, towed in off the old road that night, to this lot. He remembers sitting there a minute, and the engine coming back by itself. He does not remember a tow."), *T));
		if (D.Refresh) D.Refresh();
		if (D.Save) D.Save();
		return TArray<FS47LostLine>{
			You(TEXT("The gas station in Caliche towed a rig in off the old road last October. A tractor and a reefer.")),
			Aside(TEXT("You put the yellow carbon down by his plate. He reads it without picking it up.")),
			Driver(TEXT("That's my rig.")),
			Driver(TEXT("Nobody towed me. She came back on by herself, and I drove her in.")),
			You(TEXT("It says you were in the cab. It says he had you on this lot at twenty-five past four.")),
			Aside(TEXT("He looks at his watch for a long time.")),
			Driver(TEXT("I sat there a minute. Then she came back.")),
			Aside(TEXT("He pushes the carbon back across the counter and picks up his fork. He does not eat.")),
		};
	}
	return {};
}

void FS47LostNights::Restore()
{
	if (Has(K::Reel) && D.File) D.File(ReelPaper(Has(K::Boy)));
}

// ---------- the self test ----------

bool FS47LostNights::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bCond, const FString& What) { Lines.Add(FString::Printf(TEXT("%s lost nights: %s"), bCond ? TEXT("ok") : TEXT("FAIL"), *What)); bOk &= bCond; };
	FS47TownSeen Seen;
	TArray<FString> Links, Notes;
	bool bBinder = true;
	FS47LostNightsDeps D;
	D.Seen = &Seen;
	D.Links = [&Links]() -> TArray<FString>& { return Links; };
	D.Open = [] { return true; };
	D.Binder = [&bBinder] { return bBinder; };
	D.Notes = [&Notes] { return Notes; };
	D.Note = [&Notes](const FString& T) { Notes.Add(T); };
	D.Clock = [] { return 3 * 3600.0; };
	FS47LostNights L(D);
	Check(L.Books().Num() == 1, TEXT("on the board with the binder read"));
	FS47RuleBook& B = *L.Book;
	Check(B.Connect(TEXT("ln.b85"), TEXT("ln.g85"), false)->Text == TEXT("The night SARO printed -38, the attendant lost two hours on the old road."), TEXT("October 1985 tied"));
	Check(B.Connect(TEXT("ln.b83"), TEXT("ln.g83"), false)->Text == TEXT("The night SARO printed -36, a man in Caliche lost track of the time."), TEXT("March 1983 tied, as the town has it so far"));
	Check(!B.Connect(TEXT("ln.b81"), TEXT("ln.b83"), false)->bOk, TEXT("two printouts on the same binder are not a thread"));
	const TOptional<FS47Result> R = B.Connect(TEXT("ln.b81"), TEXT("ln.g81"), false);
	Check(R && R->Text.EndsWith(TEXT("And a question with it: -39 tonight. Who loses time tonight?")) && Seen.Contains(TEXT("ln:q39")) && Seen.Contains(TEXT("ln:81")) && Notes.Num() == 1, TEXT("the three nights tied: the question, once"));
	Check(B.Questions().Num() == 1 && B.Questions()[0].Elsewhere == TEXT("Nobody has an answer for it. Not yet."), TEXT("nothing answers it"));
	Notes.Add(TEXT("03:00 to 03:31. Nothing."));
	Check(B.Questions()[0].Elsewhere == TEXT("Under it, in my own hand:\n~ 03:00 to 03:31\nI do not remember writing it."), TEXT("after a time away, the minutes under it in the player's hand"));
	Check(L.DriverChoices(true).Num() == 0 && L.DriverOpen().Num() == 1 && L.DriverChoices(true).Num() == 1, TEXT("the driver's watch, then the question about it"));
	return bOk;
}

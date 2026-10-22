#include "S47TripRadio.h"

#include "S47Geo.h"
#include "S47Audio.h"

namespace
{
	struct FSay { FString Id; TArray<FS47RadioLine> Lines; };
	struct FMark
	{
		FString Trip;
		double At, Reach;
		TArray<FString> Ch;
		FString On;
		TArray<FSay> Say;
	};
	constexpr double REACH = 450;
	FString Cap(const FString& S) { return S.IsEmpty() ? S : S.Left(1).ToUpper() + S.Mid(1); }
	FS47RadioLine H(const FString& Text) { return FS47RadioLine(Text, TEXT("host")); }
	FS47RadioLine H(TFunction<FString(double)> Text) { return FS47RadioLine(MoveTemp(Text), TEXT("host")); }
	FS47RadioLine R(const FString& Text) { return FS47RadioLine(Text, TEXT("roswell")); }
	FS47RadioLine R(TFunction<FString(double)> Text) { return FS47RadioLine(MoveTemp(Text), TEXT("roswell")); }
	FS47RadioLine W(const FString& Text) { return FS47RadioLine(Text, TEXT("woman"), TEXT("a caller")); }
	FS47RadioLine M(const FString& Text) { return FS47RadioLine(Text, TEXT("man"), TEXT("a caller")); }
	FS47RadioLine O(const FString& Text) { return FS47RadioLine(Text, TEXT("old"), TEXT("a caller")); }
	const TArray<FString> EARLY = { TEXT("ch1"), TEXT("ch2") }, NIGHT = { TEXT("ch1"), TEXT("ch2"), TEXT("ch3") }, CH3 = { TEXT("ch3") }, MORNING = { TEXT("ch3"), TEXT("ch4"), TEXT("ch5") };

	// tripRadio.ts PROGRAMME: these marks and every line keep the source's order.
	const TArray<FMark> PROGRAMME = {
		// ---------- 1420, the Night Line: leaving SARO, any way ----------
		{ TEXT("saro>*"), 150, REACH, EARLY, TEXT("caliche"), {
			{ TEXT("out1"), {
				H([](double C) { return TEXT("Night Line, fourteen-twenty. ") + Cap(S47Radio::SayTime(C)) + TEXT(", and the coffee in here is older than some of you."); }),
				H(TEXT("Bank sign says thirty-six now. Warming up by Wednesday, they tell me.")),
			} },
			{ TEXT("out2"), {
				H([](double C) { return Cap(S47Radio::SayTime(C)) + TEXT(" on the Night Line. I can see the highway out my window, and it's empty both ways."); }),
			} },
			{ TEXT("out3"), {
				W(TEXT("Earl? I can't sleep. The wind stopped all at once tonight, like somebody shut a door.")),
				H(TEXT("I noticed that. Rattled the windows all evening, and then nothing.")),
			} },
		} },
		{ TEXT("saro>*"), 420, 300, EARLY, TEXT("caliche"), {
			{ TEXT("roof"), {
				W(TEXT("I drove past the motor court a while ago. There's somebody sitting up on the roof. Small. Real still.")),
				H(TEXT("Up on a roof at this hour, they can have it.")),
			} },
		} },
		// ---------- out to STATION 01 before chapter three: chained ----------
		{ TEXT("saro>station01"), 900, 350, EARLY, TEXT("caliche"), {
			{ TEXT("chain"), {
				O(TEXT("Earl, you know the old survey station, out the gravel west of the highway?")),
				H(TEXT("The government place. Nobody's been out there in years.")),
				O(TEXT("Somebody put a new chain on it. Shiny. Who buys a new chain for a place nobody goes?")),
			} },
		} },
		{ TEXT("saro>station01"), 1200, 300, EARLY, TEXT("caliche"), {
			{ TEXT("lamps"), {
				H(TEXT("My dad said the survey crews out there worked nights, back in forty-seven. You could see their lamps from town.")),
				H(TEXT("Then one summer they packed up and went, and left the lamps.")),
			} },
		} },
		// ---------- chapter three: the drive out to STATION 01 ----------
		{ TEXT("saro>station01"), 150, REACH, CH3, TEXT("caliche"), {
			{ TEXT("c3out"), {
				H([](double C) { return Cap(S47Radio::SayTime(C)) + TEXT(". Not long now till four, so if you've been meaning to call, call."); }),
			} },
		} },
		{ TEXT("saro>station01"), 400, 400, CH3, TEXT("caliche"), {
			{ TEXT("fence"), {
				M(TEXT("Earl, somebody's been cutting my fence on the Hondo road. Both ends, clean, like with bolt cutters.")),
				H(TEXT("Not the wind, then.")),
				M(TEXT("Wind don't carry bolt cutters.")),
			} },
		} },
		{ TEXT("saro>station01"), 850, 350, CH3, TEXT("caliche"), {
			{ TEXT("gate"), {
				W(TEXT("Earl, it's Dolores again. There's a light on out at the old survey station.")),
				H(TEXT("County chained that gate years ago, Dolores.")),
				W(TEXT("The big gate. Not the little one beside it.")),
			} },
		} },
		{ TEXT("saro>station01"), 1150, 300, CH3, TEXT("caliche"), {
			{ TEXT("marks"), {
				H(TEXT("My granddad ran chain for the survey crews. Never trust one mark, he said. Sight two, and if one has walked, the other tells you.")),
				H(TEXT("And if you think it's your own lamp making the shadows, put your hat over the lamp and look again.")),
			} },
		} },
		// ---------- chapter three: back to SARO with the film (before four, or after five) ----------
		{ TEXT("station01>saro"), 150, 550, CH3, TEXT("caliche"), {
			{ TEXT("film"), {
				H(TEXT("My sister ran the photo counter at the drugstore twenty years. Film doesn't care what time it is, she used to say.")),
				H(TEXT("But it doesn't keep, either. Not in a warm truck.")),
			} },
		} },
		{ TEXT("station01>saro"), 900, 600, CH3, TEXT("caliche"), {
			{ TEXT("late"), {
				H(TEXT("That's about all from the Night Line. When the anthem comes on, that's me going home.")),
			} },
		} },
		{ TEXT("station01>saro"), 150, 850, MORNING, TEXT("roswell"), {
			{ TEXT("mfilm"), {
				R(TEXT("The camera shop on Main opens at nine. Film left in a warm car is film you won't like.")),
			} },
		} },
		// ---------- back from the station before chapter three ----------
		{ TEXT("station01>*"), 150, REACH, EARLY, TEXT("caliche"), {
			{ TEXT("track"), {
				M(TEXT("Earl, I drove out the survey track tonight. Just to see.")),
				H(TEXT("And?")),
				M(TEXT("Chained, like you said. And it took me the longest time to find the highway again. It's one road, Earl.")),
			} },
		} },
		// ---------- down the highway to the diner and Caliche, at night ----------
		{ TEXT("saro>diner"), 1000, 500, NIGHT, TEXT("caliche"), {
			{ TEXT("oldroad"), {
				O(TEXT("Earl, is the old Roswell road open past the diner yet?")),
				H(TEXT("Not past the junction. Barricade's still up. You want Roswell, go around like everybody else.")),
			} },
			{ TEXT("probe"), {
				H(TEXT("That European probe flew right up to the comet last month and sent back a picture of a potato. Seventy-six years we waited, for a potato.")),
			} },
		} },
		{ TEXT("saro>diner"), 1600, 450, NIGHT, TEXT("caliche"), {
			{ TEXT("coffee"), {
				W(TEXT("Earl, I'm down at the Mesa. She says tell you the coffee's fresh and the pie isn't.")),
				H(TEXT("Tell her I heard. And tell her I'm still not paying for last week's.")),
			} },
			{ TEXT("tax"), {
				H(TEXT("Tuesday is tax day, folks. If you haven't mailed it yet, I don't want to hear about it on this program.")),
			} },
		} },
		{ TEXT("saro>diner"), 2200, 700, NIGHT, TEXT("caliche"), {
			{ TEXT("door"), {
				H(TEXT("If you're in town and you see the light on over my door, I'm in here. Lean on the buzzer during a long record.")),
			} },
		} },
		// ---------- leaving the diner or Caliche, at night ----------
		{ TEXT("diner>*"), 150, 450, NIGHT, TEXT("caliche"), {
			{ TEXT("watch"), {
				W(TEXT("Earl, there's a light sitting over the mesa east of town. It isn't a star. Stars don't sit that low.")),
				H(TEXT("Is it doing anything?")),
				W(TEXT("It's watching the road.")),
				H(TEXT("Well. Wave.")),
			} },
			{ TEXT("pumps"), {
				H([](double C) { return Cap(S47Radio::SayTime(C)) + TEXT(" in Caliche. Pumps are locked till six, so if you're low, you're low."); }),
			} },
		} },
		{ TEXT("diner>saro"), 1600, 600, NIGHT, TEXT("caliche"), {
			{ TEXT("dishes"), {
				M(TEXT("Earl, those big dishes up at the observatory. Did they all turn at once tonight?")),
				H(TEXT("They do that. They're listening for something.")),
				M(TEXT("A little after two. All of them together, like sunflowers.")),
			} },
			{ TEXT("cattle"), {
				H(TEXT("Cattle report, for those of you who keep cattle and can't sleep. Feeders off again at the Roswell sale. Everybody still blames the dairy buyout.")),
			} },
		} },
		// ---------- 1230, Roswell, after five: chapter five's drive to the diner ----------
		{ TEXT("saro>diner"), 250, 700, MORNING, TEXT("roswell"), {
			{ TEXT("m1"), {
				R([](double C) { return Cap(S47Radio::SayTime(C)) + TEXT(" on a Monday morning, April the fourteenth. Here's the farm and ranch report."); }),
				R(TEXT("Feeder steers a dollar lower at the sale barn. The cattlemen are still sore about the dairy buyout, and who can blame them.")),
			} },
		} },
		{ TEXT("saro>diner"), 1000, 500, MORNING, TEXT("roswell"), {
			{ TEXT("m2"), {
				R(TEXT("Tuesday is tax day. The post office in Roswell keeps the window open till midnight tomorrow, for the brave.")),
			} },
		} },
		{ TEXT("saro>diner"), 1600, 500, MORNING, TEXT("roswell"), {
			{ TEXT("m3"), {
				R(TEXT("Mesa Diner at the old road junction has been open all night, and the coffee is on. Tell her the radio sent you.")),
			} },
		} },
		{ TEXT("saro>diner"), 2150, 700, MORNING, TEXT("roswell"), {
			{ TEXT("m4"), {
				R(TEXT("Sunrise at five thirty. If you're headed east this morning, the sun will be right in your eyes. Take it slow out there.")),
			} },
		} },
		// ---------- 1230, other ways in the morning ----------
		{ TEXT("diner>saro"), 1500, 700, MORNING, TEXT("roswell"), {
			{ TEXT("m5"), {
				R(TEXT("Clear and cold this morning, and the wind should stay down till noon. Then it's spring in New Mexico again, and hold on to your hat.")),
			} },
		} },
		{ TEXT("diner>*"), 150, 550, MORNING, TEXT("roswell"), {
			{ TEXT("m6"), {
				R(TEXT("The comet is past its best now. Next time around is twenty sixty-one. I'll let my grandkids have that one.")),
			} },
		} },
	};

	// tripRadio.ts place: motel and a developer start on the road count as SARO.
	FString Place(const FString& P) { return P == TEXT("motel") || P == TEXT("road") ? FString(TEXT("saro")) : P == TEXT("roswell") ? FString() : P; }
	const TMap<FString, double> START = { { TEXT("motel"), S47Geo::MOTEL_AT().Z } };
}

// tripRadio.ts programmeSize.
int32 FS47TripRadio::ProgrammeSize()
{
	int32 N = 0;
	for (const FMark& Mark : PROGRAMME) for (const FSay& Say : Mark.Say) N += Say.Lines.Num();
	return N;
}

FS47TripRadio::FS47TripRadio(FS47Radio& InRadio, TSet<FString>& InSeen) : Radio(InRadio), Seen(InSeen)
{
	// TripRadio.plan: the first fixed text of each say, and its by (or null).
	for (const FMark& Mark : PROGRAMME)
	{
		FS47TripRadioPlan P;
		P.Trip = Mark.Trip; P.At = Mark.At; P.Reach = Mark.Reach; P.Ch = Mark.Ch; P.On = Mark.On;
		for (const FSay& Say : Mark.Say)
		{
			const FS47RadioLine* Fixed = Say.Lines.FindByPredicate([](const FS47RadioLine& L) { return !L.TextAt; });
			P.Ids.Add(Say.Id); P.First.Add(Fixed ? Fixed->Text : FString());
			P.By.Add(Fixed && !Fixed->By.IsEmpty() ? TOptional<FString>(Fixed->By) : TOptional<FString>());
		}
		Plan.Add(MoveTemp(P));
	}
	Lines = ProgrammeSize();
}

// TripRadio.update: the station takes a due mark, and a waiting one goes back if the trip ends.
void FS47TripRadio::Update(const FS47TripRadioIn& S)
{
	const TOptional<FS47TripNow>& Tr = S.Trip;
	const TOptional<FTaken> Tk = Taken;
	const FString At = Tk.IsSet() ? Tk->St->Where(Tk->X) : FString();
	if (Tk.IsSet() && At == TEXT("on")) { Last = Tk->On; Taken.Reset(); }
	else if (Tk.IsSet() && (At.IsEmpty() || !Tr.IsSet() || !Trip.IsSet() || Trip->N != Tr->N))
	{
		Tk->St->Unslot(Tk->X); Seen.Remove(TEXT("tr:") + Tk->On.Id);
		Taken.Reset();
	}
	if (!Tr.IsSet()) { Trip.Reset(); return; }
	if (!Trip.IsSet() || Trip->N != Tr->N) Trip = FTrip { Tr->N, {} };
	if (S.Tuned.IsEmpty() || Taken.IsSet()) return;
	const FString From = Place(Tr->From), To = Place(Tr->Bound);
	const double* Start = START.Find(Tr->From);
	const double Metres = Tr->Metres + (Start ? *Start : 0);
	const TSharedPtr<FS47Station> St = Radio.Stations.FindChecked(S.Tuned);
	TSet<int32>& Done = Trip->Done;
	for (int32 I = 0; I < PROGRAMME.Num(); ++I)
	{
		const FMark& Mark = PROGRAMME[I];
		if (Done.Contains(I) || Mark.On != S.Tuned || Metres < Mark.At || !Mark.Ch.Contains(S.Phase)) continue;
		FString A, B; Mark.Trip.Split(TEXT(">"), &A, &B);
		if ((A != TEXT("*") && A != From) || (B != TEXT("*") && B != To)) continue;
		if (Metres > Mark.At + Mark.Reach) { Done.Add(I); continue; }
		const FSay* Say = Mark.Say.FindByPredicate([this](const FSay& X) { return !Seen.Contains(TEXT("tr:") + X.Id); });
		if (!Say) { Done.Add(I); continue; }
		const TSharedPtr<FS47RadioSeg> X = St->Slot(Say->Lines, S.Clock);
		if (X)
		{
			Done.Add(I); Seen.Add(TEXT("tr:") + Say->Id);
			Taken = FTaken { St, X, { Say->Id, Metres, S.Clock, From + TEXT(">") + (To.IsEmpty() ? TEXT("?") : To) } };
		}
		return;
	}
}

bool FS47TripRadio::SelfTest(TArray<FString>& Out)
{
	bool bOk = true;
	auto Check = [&Out, &bOk](bool bPass, const TCHAR* What) { Out.Add(FString::Printf(TEXT("%s tripRadio: %s"), bPass ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= bPass; };
	US47Audio* Audio = NewObject<US47Audio>();
	FS47Radio R(Audio);
	TSet<FString> Seen;
	FS47TripRadio P(R, Seen);
	Check(P.Lines == 51 && P.Plan.Num() == 23, TEXT("23 marks and 51 lines"));
	Check(P.Plan[0].Ids == TArray<FString> { TEXT("out1"), TEXT("out2"), TEXT("out3") } && P.Plan.Last().Ids[0] == TEXT("m6"), TEXT("mark and alternative order"));
	Check(P.Plan[0].First[0] == TEXT("Bank sign says thirty-six now. Warming up by Wednesday, they tell me.") && !P.Plan[0].By[0].IsSet(), TEXT("plan skips a variable time check"));
	Check(P.Plan[1].By[0].IsSet() && *P.Plan[1].By[0] == TEXT("a caller") && P.Plan[4].First[0].IsEmpty(), TEXT("plan keeps caller attribution and an all-variable say"));
	TSet<FString> Ids;
	bool bUnique = true;
	for (const FMark& Mark : PROGRAMME) for (const FSay& Say : Mark.Say) { bUnique &= !Ids.Contains(Say.Id); Ids.Add(Say.Id); }
	Check(bUnique && Ids.Num() == 29, TEXT("29 unique once-per-night IDs"));
	Check(Place(TEXT("motel")) == TEXT("saro") && Place(TEXT("road")) == TEXT("saro") && Place(TEXT("roswell")).IsEmpty() && Place(FString()).IsEmpty(), TEXT("place aliases and old-road exclusion"));
	Check(PROGRAMME[0].Say[0].Lines[0].At(2 * 3600).StartsWith(TEXT("Night Line, fourteen-twenty. ")) && PROGRAMME[0].Say[0].Lines[0].At(2 * 3600) != PROGRAMME[0].Say[0].Lines[0].At(3 * 3600), TEXT("time checks are evaluated at the programme clock"));

	const TSharedPtr<FS47Station> St = R.Stations.FindChecked(TEXT("caliche"));
	auto Reset = [&R, &St](double Clock = 2 * 3600) { R.Reset(Clock); St->T = 6; };
	FS47TripRadioIn S;
	S.Trip = FS47TripNow { 1, TEXT("saro"), TEXT("diner"), TEXT("diner"), 149 };
	S.Phase = TEXT("ch1"); S.Clock = 2 * 3600; S.Tuned = TEXT("caliche");
	Reset(); P.Update(S);
	Check(Seen.IsEmpty() && !P.Taken.IsSet(), TEXT("mark does not start before 150 metres"));
	S.Trip->Metres = 150; P.Update(S);
	Check(Seen.Contains(TEXT("tr:out1")) && P.Taken.IsSet() && !P.Last.IsSet(), TEXT("station takes first due mark, last waits for broadcast"));
	if (P.Taken.IsSet()) Check(St->Where(P.Taken->X) == TEXT("waiting"), TEXT("taken mark is queued behind the current programme"));
	S.Tuned = TEXT("far"); P.Update(S);
	Check(P.Taken.IsSet() && Seen.Num() == 1, TEXT("tuning away keeps the waiting slot on the same trip"));
	P.Update({});
	Check(!P.Taken.IsSet() && Seen.IsEmpty() && !P.Trip.IsSet(), TEXT("leaving before broadcast returns the mark for another drive"));

	Reset(); S.Tuned = TEXT("caliche"); S.Trip->N = 2; P.Update(S);
	if (P.Taken.IsSet()) St->Queue = { P.Taken->X };
	P.Update(S);
	Check(P.Last.IsSet() && P.Last->Id == TEXT("out1") && P.Last->Metres == 150 && P.Last->Trip == TEXT("saro>diner") && Seen.Contains(TEXT("tr:out1")), TEXT("on-air slot becomes last and stays seen"));
	P.Update({}); Reset(); S.Trip->N = 3; P.Update(S);
	Check(P.Taken.IsSet() && P.Taken->On.Id == TEXT("out2") && Seen.Contains(TEXT("tr:out2")), TEXT("next trip takes the next unseen alternative"));
	Reset(4.5 * 3600); S.Clock = 4.5 * 3600; P.Update(S);
	Check(!P.Taken.IsSet() && !Seen.Contains(TEXT("tr:out2")) && Seen.Contains(TEXT("tr:out1")), TEXT("sign-off discards only the unbroadcast reservation"));

	P.Update({}); Seen.Empty(); Reset(); S.Clock = 2 * 3600; S.Trip->N = 4; S.Trip->Metres = 800;
	P.Update(S);
	Check(!P.Taken.IsSet() && Seen.IsEmpty() && P.Trip.IsSet() && P.Trip->Done.Contains(0) && P.Trip->Done.Contains(1), TEXT("driving past a window marks it done for this trip"));
	S.Trip->Metres = 1000; P.Update(S);
	Check(P.Taken.IsSet() && P.Taken->On.Id == TEXT("oldroad"), TEXT("specific destination mark follows expired general marks"));

	P.Update({}); Seen.Empty(); Reset(); S.Trip = FS47TripNow { 5, TEXT("motel"), TEXT("diner"), TEXT("diner"), 0 };
	P.Update(S);
	Check(P.Taken.IsSet() && P.Taken->On.Id == TEXT("roof") && P.Taken->On.Metres == S47Geo::MOTEL_AT().Z, TEXT("motel start adds its 650 metres"));
	P.Update({}); Seen.Empty(); Reset(); S.Trip = FS47TripNow { 6, TEXT("saro"), TEXT("diner"), TEXT("diner"), 150 };
	S.Phase = TEXT("ch4"); P.Update(S);
	Check(!P.Taken.IsSet() && Seen.IsEmpty(), TEXT("chapter restriction keeps early Night Line marks out of chapter four"));

	P.Update({}); Reset(5.25 * 3600); R.Stations.FindChecked(TEXT("roswell"))->T = 6;
	S.Trip = FS47TripNow { 7, TEXT("saro"), TEXT("diner"), TEXT("diner"), 250 };
	S.Phase = TEXT("ch5"); S.Clock = 5.25 * 3600; S.Tuned = TEXT("roswell"); P.Update(S);
	Check(P.Taken.IsSet() && P.Taken->On.Id == TEXT("m1") && Seen.Contains(TEXT("tr:m1")), TEXT("morning station takes its chapter-five farm report"));
	P.Update({});
	return bOk;
}

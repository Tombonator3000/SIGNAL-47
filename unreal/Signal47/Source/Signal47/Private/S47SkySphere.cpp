#include "S47SkySphere.h"
#include "S47Observing.h"

// RX-LOOK-20261113 (-> M3, M6): see S47SkySphere.h. Only the self-test lives here; the table and the arithmetic are in the header.
bool S47SkySphere::SelfTest(TArray<FString>& Lines)
{
	using FSky = FS47SkyAcquire;
	bool bOk = true;
	auto Check = [&](bool bPass, const FString& What) { bOk &= bPass; Lines.Add(FString(bPass ? TEXT("PASS ") : TEXT("FAIL ")) + TEXT("sky sphere: ") + What); };
	auto Apart = [](double RaA, double DecA, double RaB, double DecB) { return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Dir(RaA, DecA), Dir(RaB, DecB)), -1., 1.))); };

	{ // the table
		const int32 Count = int32(UE_ARRAY_COUNT(Stars));
		bool bRange = Count == int32(Star::Count), bApart = true, bLinks = true;
		double Longest = 0, Nearest = 1e9;
		for (int32 I = 0; I < Count; ++I)
		{
			bRange &= Stars[I].RaHours >= 0 && Stars[I].RaHours < 24 && Stars[I].Dec >= -50 && Stars[I].Dec <= 90 && Stars[I].Mag >= -1.5f && Stars[I].Mag <= 4.6f;
			for (int32 K = I + 1; K < Count; ++K) Nearest = FMath::Min(Nearest, Apart(Stars[I].RaHours * 15., Stars[I].Dec, Stars[K].RaHours * 15., Stars[K].Dec));
		}
		bApart = Nearest > .4;
		for (const FLink& Link : Links)
		{
			bLinks &= Link.A >= 0 && Link.A < Count && Link.B >= 0 && Link.B < Count && Link.A != Link.B;
			if (bLinks) Longest = FMath::Max(Longest, Apart(Stars[Link.A].RaHours * 15., Stars[Link.A].Dec, Stars[Link.B].RaHours * 15., Stars[Link.B].Dec));
		}
		Check(bRange && bApart && bLinks && Count >= 150 && Count <= 250 && Longest < 24.,
			FString::Printf(TEXT("%d stars, no two the same (nearest %.1f degrees apart), %d lines of the figures between stars of the table (longest %.1f degrees)"), Count, Nearest, int32(UE_ARRAY_COUNT(Links)), Longest));
		// a few everyone knows, where the catalogues have them
		const FStar& Vega = Stars[Star::AlpLyr]; const FStar& Sirius = Stars[Star::AlpCMa]; const FStar& Polaris = Stars[Star::AlpUMi];
		Check(Apart(Vega.RaHours * 15., Vega.Dec, 279.23, 38.78) < .2 && Apart(Sirius.RaHours * 15., Sirius.Dec, 101.29, -16.72) < .2 && Polaris.Dec > 89. && Sirius.Mag < Vega.Mag && Vega.Mag < Polaris.Mag,
			TEXT("Vega, Sirius and Polaris stand where the catalogues have them, the brightest first"));
	}
	{ // the ground and the sky
		bool bRound = true;
		for (double Az = FSky::AzMin; Az <= FSky::AzMax; Az += 7.) for (double El = FSky::ElMin; El <= FSky::ElMax; El += 6.5)
		{
			double Ha = 0, Dec = 0;
			AzElToHaDec(FVector2D(Az, El), Ha, Dec);
			bRound &= HaDecToAzEl(Ha, Dec).Equals(FVector2D(Az, El), 1e-8);
			double HaHours = 0, DecRule = 0;
			FSky::HourAngleDec(FVector2D(Az, El), HaHours, DecRule);
			bRound &= FMath::Abs(HaHours * 15. - Ha) < 1e-9 && FMath::Abs(DecRule - Dec) < 1e-9; // the rule's own readout
		}
		double Ha = 9, Dec = 0;
		AzElToHaDec(FVector2D(77, 90), Ha, Dec);
		Check(bRound && FMath::Abs(Dec - FSky::Latitude) < 1e-6 && HaDecToAzEl(0., 0.).Equals(FVector2D(180., 90. - FSky::Latitude), 1e-6),
			TEXT("azimuth and elevation to hour angle and declination and back, as the rule's readout has them; the zenith at the station's latitude, the equator on the meridian due south"));
		// the clock: Greenwich's sidereal time at the J2000 epoch, and the game's own evening
		const double Evening = LstOf(1986, 4, 13, 22 * 3600. + 5 * 60.);
		Check(FMath::Abs(GmstDeg(2451545.) - 280.4606) < 1e-3 && FMath::Abs(JulianDay(2000, 1, 1) - 2451544.5) < 1e-9 && FMath::Abs(JulianDay(1986, 4, 13) - 2446533.5) < 1e-9 && FMath::Abs(Evening - 171.7) < .3,
			FString::Printf(TEXT("sidereal time from the date and the Mountain clock (22:05 on 13 April 1986: %.2f h at the station)"), Evening / 15.));
		// the cards were written from the real sky: the terminal's clock puts each source where its card points
		const TArray<FS47Observing::FRun>& RunList = FS47Observing::Runs();
		bool bCards = true; double Worst = 0;
		for (const FKnown& Src : Known)
		{
			if (!RunList.IsValidIndex(Src.Run)) { bCards = false; continue; }
			double Ra = Src.RaDeg, DecNow = Src.DecDeg;
			FromJ2000(Ra, DecNow);
			const FVector2D Stands = RaDecToAzEl(Ra, DecNow, LstOf(1986, 4, 13, Src.ClockHours * 3600.));
			const FVector2D Card((RunList[Src.Run].P.Az[0] + RunList[Src.Run].P.Az[1]) * .5, RunList[Src.Run].El);
			const double Off = FVector2D::Distance(Stands, Card);
			Worst = FMath::Max(Worst, Off);
			bCards &= Off < 6.;
		}
		Check(bCards, FString::Printf(TEXT("3C 286, the pulsar and Virgo A stand where their cards point at their scheduled hours on 13 April 1986 (farthest %.1f degrees in azimuth and elevation)"), Worst));
		// a star crosses the meridian when the sidereal time is its right ascension; the pole star hardly moves
		const FStar& Vega = Stars[Star::AlpLyr]; const FStar& Polaris = Stars[Star::AlpUMi];
		const FVector2D Transit = RaDecToAzEl(Vega.RaHours * 15., Vega.Dec, Vega.RaHours * 15.);
		bool bPole = true;
		for (double Lst = 0; Lst < 360; Lst += 30) { const FVector2D At = RaDecToAzEl(Polaris.RaHours * 15., Polaris.Dec, Lst); bPole &= FMath::Abs(At.Y - FSky::Latitude) < .8 && (At.X < 1.5 || At.X > 358.5); }
		const FVector2D Before = RaDecToAzEl(Vega.RaHours * 15., Vega.Dec, Vega.RaHours * 15. - 30.), After = RaDecToAzEl(Vega.RaHours * 15., Vega.Dec, Vega.RaHours * 15. + 30.);
		Check(FMath::Abs(Transit.Y - (90. - (Vega.Dec - FSky::Latitude))) < 1e-6 && bPole && Before.X < 180 && After.X > 180,
			TEXT("Vega is highest when the sidereal time is its right ascension, east of the meridian before and west after; Polaris stays at the pole"));
	}
	{ // epochs and the galaxy
		double Ra = 279.23, Dec = 38.78;
		Precess(Ra, Dec, -50.);
		double BackRa = Ra, BackDec = Dec;
		Precess(BackRa, BackDec, 50.);
		double Lon = 0, Lat = 0, PoleLon = 0, PoleLat = 0, AgainRa = 0, AgainDec = 0;
		EqToGal(266.405, -28.936, Lon, Lat);
		EqToGal(192.8595, 27.1283, PoleLon, PoleLat);
		GalToEq(123., -17., AgainRa, AgainDec);
		double AgainLon = 0, AgainLat = 0;
		EqToGal(AgainRa, AgainDec, AgainLon, AgainLat);
		Check(FMath::Abs(Ra - 278.81) < .03 && FMath::Abs(Dec - 38.74) < .03 && FMath::Abs(BackRa - 279.23) < .01 && FMath::Abs(BackDec - 38.78) < .01,
			FString::Printf(TEXT("Vega for B1950.0 at %s %s, as the catalogues of the day print it, and back"), *RaText(Ra), *DecText(Dec)));
		Check(FMath::Min(Lon, 360. - Lon) < .02 && FMath::Abs(Lat) < .02 && PoleLat > 89.98 && FMath::Abs(AgainLon - 123.) < 1e-6 && FMath::Abs(AgainLat + 17.) < 1e-6,
			TEXT("galactic longitude and latitude: the centre at zero, the pole at ninety, and back"));
		bool bDust = true; int32 Toward = 0;
		for (int32 I = 0; I < DustCount; ++I)
		{
			const FDust A = Dust(I), B = Dust(I);
			bDust &= A.Lon == B.Lon && A.Lat == B.Lat && A.Bright == B.Bright && A.Lon >= 0 && A.Lon < 360 && FMath::Abs(A.Lat) <= 12.5f && A.Bright > 0 && A.Bright <= 1;
			Toward += A.Lon < 60 || A.Lon > 300 ? 1 : 0;
		}
		Check(bDust && Toward * 3 > DustCount, FString::Printf(TEXT("the galactic plane's %d specks are the same every time, within twelve degrees of the plane, most of them toward the centre (%d)"), DustCount, Toward));
		Check(RaText(152.1).StartsWith(TEXT("10h08m")) && DecText(-0.5) == TEXT("-00°30'") && DecText(19.55) == TEXT("+19°33'"), TEXT("the readouts' words for a right ascension and a declination"));
	}
	{ // the picture
		bool bAll = true; double LockLeast = 1e9, LockMost = 0, Slack = 1e9;
		FString Sizes;
		for (const FVector2D Semi : { FVector2D(314, 203), FVector2D(300, 150), FVector2D(220, 200), FVector2D(480, 300) })
		{
			const FVector2D RimMid(Semi.X + 40., Semi.Y + 12.);
			const FView V = FitView(RimMid, Semi);
			bool bRound = true, bIn = true; double Least = 1e9, Most = 0, Margin = 1e9;
			for (double Az = FSky::AzMin; Az <= FSky::AzMax + 1e-9; Az += 2.) for (double El = FSky::ElMin; El <= FSky::ElMax + 1e-9; El += 2.)
			{
				FVector2D P;
				if (!ProjectAzEl(V, FVector2D(Az, El), P)) { bIn = false; continue; }
				bRound &= UnprojectAzEl(V, P).Equals(FVector2D(Az, El), 1e-6);
				bIn &= Inside(V, P);
				const double Rx = (P.X - V.Rim.X) / V.Semi.X, Ry = (P.Y - V.Rim.Y) / V.Semi.Y;
				Margin = FMath::Min(Margin, 1. - FMath::Sqrt(Rx * Rx + Ry * Ry));
				// the rule's lock circle on the glass: its narrowest and widest half-width here
				for (double Turn = 0; Turn < 180; Turn += 15)
				{
					const double Units = UnitsPerDegree(V, FVector2D(Az, El), FVector2D(FMath::Cos(FMath::DegreesToRadians(Turn)), FMath::Sin(FMath::DegreesToRadians(Turn)))) * FSky::LockRadius;
					Least = FMath::Min(Least, Units); Most = FMath::Max(Most, Units);
				}
			}
			FVector2D AtCentre;
			const bool bCentre = ProjectAzEl(V, FVector2D((FSky::AzMin + FSky::AzMax) * .5, (FSky::ElMin + FSky::ElMax) * .5), AtCentre) && AtCentre.Equals(V.Mid, 1e-6);
			bAll &= bRound && bIn && bCentre && Margin > 1. - RimMargin - 1e-6 && Margin < .12;
			Slack = FMath::Min(Slack, Margin);
			if (Semi.X == 314) { LockLeast = Least; LockMost = Most; }
			Sizes += FString::Printf(TEXT(" %.0fx%.0f: %.2f u/deg"), Semi.X * 2, Semi.Y * 2, FMath::DegreesToRadians(V.Radius));
		}
		Check(bAll, FString::Printf(TEXT("the array's whole window lies inside the rim at every size, to the glass and back to a millionth of a degree (nearest the rim %.3f of it;%s)"), Slack, *Sizes));
		const FView V = FitView(FVector2D(354, 215), FVector2D(314, 203));
		// (The old flat chart gave the lock radius 9.9 units every way. On a sphere a degree of azimuth is cos(elevation) degrees of
		// arc, so it is narrower across high up. SS47SkyChart closes the picture in on the crossing as the pointer or the reticle
		// nears it, which makes up for it; the keys steer in the rule's degrees either way.)
		double AtCards = 1e9;
		for (const FSky::FSource& Src : FSky::Catalogue())
		{
			if (!Src.bMajor) continue;
			for (double Turn = 0; Turn < 180; Turn += 15) AtCards = FMath::Min(AtCards, UnitsPerDegree(V, Src.At, FVector2D(FMath::Cos(FMath::DegreesToRadians(Turn)), FMath::Sin(FMath::DegreesToRadians(Turn)))) * FSky::LockRadius);
		}
		Check(LockLeast >= 1.8 && LockMost <= 11. && AtCards >= 3.2, FString::Printf(TEXT("the rule's lock radius is %.1f to %.1f units on the open picture across the window, %.1f at the narrowest card source (the flat chart: 9.9 every way)"), LockLeast, LockMost, AtCards));
		// north up, east to the left: the sky as seen, not a globe from outside
		const FVector2D Centre((FSky::AzMin + FSky::AzMax) * .5, (FSky::ElMin + FSky::ElMax) * .5);
		double Ha0 = 0, Dec0 = 0;
		AzElToHaDec(Centre, Ha0, Dec0);
		FVector2D Here, North, East;
		const bool bSeen = ProjectHaDec(V, Ha0, Dec0, Here) && ProjectHaDec(V, Ha0, Dec0 + 1., North) && ProjectHaDec(V, Ha0 - 1., Dec0, East);
		Check(bSeen && (bZenithUp || (North.Y < Here.Y - 1. && FMath::Abs(North.X - Here.X) < .05 && East.X < Here.X - 1. && FMath::Abs(East.Y - Here.Y) < .2)),
			TEXT("celestial north is up and east to the left at the middle: the sky as it is seen"));
		// the picture agrees with the sphere: a short step anywhere in the window is as long on the glass as its arc, within the
		// perspective's stretch
		const double PerDegree = FMath::DegreesToRadians(V.Radius);
		double Low = 1e9, High = 0;
		for (double Az = FSky::AzMin + 2; Az <= FSky::AzMax - 2; Az += 6.) for (double El = FSky::ElMin + 2; El <= FSky::ElMax - 2; El += 5.) for (const FVector2D Step : { FVector2D(1.6, 0), FVector2D(0, 1.6), FVector2D(1.13, 1.13) })
		{
			FVector2D A, B;
			if (!ProjectAzEl(V, FVector2D(Az, El), A) || !ProjectAzEl(V, FVector2D(Az, El) + Step, B)) continue;
			double HaA = 0, DecA = 0, HaB = 0, DecB = 0;
			AzElToHaDec(FVector2D(Az, El), HaA, DecA); AzElToHaDec(FVector2D(Az, El) + Step, HaB, DecB);
			const double Arc = Apart(HaA, DecA, HaB, DecB), Ratio = FVector2D::Distance(A, B) / FMath::Max(Arc, 1e-9) / PerDegree;
			Low = FMath::Min(Low, Ratio); High = FMath::Max(High, Ratio);
		}
		Check(Low > .8 && High < 1.08 * V.Stretch, FString::Printf(TEXT("a step on the glass is %.2f to %.2f of its arc on the sphere (the picture is %.2f wider than high)"), Low, High, V.Stretch));
		// the keys: up on the glass is up, wherever the reticle stands, at the rule's own rate
		bool bSteer = true;
		for (double Az = FSky::AzMin + 4; Az <= FSky::AzMax - 4; Az += 9.) for (double El = FSky::ElMin + 4; El <= FSky::ElMax - 4; El += 8.) for (const FVector2D Want : { FVector2D(0, 1), FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, -1) })
		{
			const FVector2D Step = SteerOnGlass(V, FVector2D(Az, El), Want);
			FVector2D A, B;
			bSteer &= FMath::IsNearlyEqual(Step.Size(), 1., 1e-6) && ProjectAzEl(V, FVector2D(Az, El), A) && ProjectAzEl(V, FVector2D(Az, El) + Step * .2, B);
			const FVector2D Moved = FVector2D(B.X - A.X, A.Y - B.Y).GetSafeNormal();
			bSteer &= FVector2D::DotProduct(Moved, Want) > .995;
		}
		Check(bSteer && SteerOnGlass(V, Centre, FVector2D::ZeroVector).IsZero(), TEXT("a steering key moves the reticle the way it points on the glass, one of the rule's degrees for a degree"));
		// nothing is drawn outside the rim: any segment cut to it lies inside
		bool bCut = true; int32 Kept = 0;
		for (uint32 I = 0; I < 400; ++I)
		{
			FVector2D A(V.Rim.X + (Hash01(I * 4 + 1) - .5) * V.Semi.X * 3.2, V.Rim.Y + (Hash01(I * 4 + 2) - .5) * V.Semi.Y * 3.2), B(V.Rim.X + (Hash01(I * 4 + 3) - .5) * V.Semi.X * 3.2, V.Rim.Y + (Hash01(I * 4 + 4) - .5) * V.Semi.Y * 3.2);
			const bool bAIn = Inside(V, A), bBIn = Inside(V, B);
			bool bCutA = false, bCutB = false;
			if (!CutToRim(V, A, B, bCutA, bCutB)) { bCut &= !bAIn && !bBIn; continue; }
			++Kept;
			bCut &= Inside(V, A, 1.0001) && Inside(V, B, 1.0001) && bCutA == !bAIn && bCutB == !bBIn;
		}
		Check(bCut && Kept > 100 && Kept < 400, FString::Printf(TEXT("a line is drawn only inside the rim (%d of 400 tried lines kept, each cut to it)"), Kept));
		// the pointer: anywhere inside the rim gives a direction, and the rule keeps it on its chart
		bool bPointer = true;
		for (uint32 I = 0; I < 300; ++I)
		{
			const FVector2D P(V.Rim.X + (Hash01(I * 2 + 900) - .5) * V.Semi.X * 2., V.Rim.Y + (Hash01(I * 2 + 901) - .5) * V.Semi.Y * 2.);
			if (!Inside(V, P)) continue;
			const FVector2D AzEl = UnprojectAzEl(V, P);
			bPointer &= FMath::IsFinite(AzEl.X) && FMath::IsFinite(AzEl.Y) && FSky::ClampToChart(AzEl).X >= FSky::AzMin && FSky::ClampToChart(AzEl).Y <= FSky::ElMax;
		}
		Check(bPointer, TEXT("the pointer anywhere inside the rim is a direction the rule can keep on its chart"));
	}
	return bOk;
}

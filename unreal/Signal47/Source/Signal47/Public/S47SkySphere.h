#pragma once
#include "CoreMinimal.h"
#include "S47SkyAcquire.h"

// RX-LOOK-20261113 (-> M3, M6; Tom 13 November: «Få receiver til grafisk å se mer ut som bilde nr. to. Masse FX.»): the receiver's
// celestial sphere as plain C++ (no Slate, no world). FS47SkyAcquire is the rule and works in azimuth and elevation; nothing here
// changes it. This is only where a direction stands on the picture and what else stands there: the equatorial grid, a table of real
// bright stars with a handful of stick figures, the galactic plane, and the numbers the readouts show (right ascension and
// declination for epoch B1950.0, galactic longitude and latitude), all worked out from the station's latitude and the game's clock.
//
// The picture is fixed to the ground, as the array's window is: an azimuth and elevation always land on the same place of the
// glass, so the reticle, the catalogue and the bearings stand where the rule has them. What turns is the sky: the stars and the
// hours of right ascension drift west at the sidereal rate as the game's clock runs.
//
// One thing is not as on the old flat chart, and cannot be on a sphere: a degree of azimuth is cos(elevation) degrees of arc, so
// the rule's lock circle (plain degrees of azimuth and elevation) is narrower across on the glass the higher the source stands.
//
// It is a schematic on a terminal: nothing here reads or draws the game's star sky.

// name, right ascension (hours, minutes), declination (degrees), magnitude. J2000, catalogue places to about a tenth of a degree.
#define S47_SKY_STAR_TABLE(X) \
	X(AlpOri, 5, 55.2, 7.41, .45) X(BetOri, 5, 14.5, -8.20, .18) X(GamOri, 5, 25.1, 6.35, 1.64) X(DelOri, 5, 32.0, -.30, 2.25) \
	X(EpsOri, 5, 36.2, -1.20, 1.69) X(ZetOri, 5, 40.8, -1.94, 1.74) X(KapOri, 5, 47.8, -9.67, 2.07) X(LamOri, 5, 35.1, 9.93, 3.40) \
	X(IotOri, 5, 35.4, -5.91, 2.75) X(Pi3Ori, 4, 49.8, 6.96, 3.19) X(EtaOri, 5, 24.5, -2.40, 3.35) \
	X(AlpTau, 4, 35.9, 16.51, .87) X(BetTau, 5, 26.3, 28.61, 1.65) X(ZetTau, 5, 37.6, 21.14, 2.97) X(EtaTau, 3, 47.5, 24.11, 2.85) \
	X(EpsTau, 4, 28.6, 19.18, 3.53) X(GamTau, 4, 19.8, 15.63, 3.65) X(LamTau, 4, 0.7, 12.49, 3.41) \
	X(AlpAur, 5, 16.7, 46.00, .08) X(BetAur, 5, 59.5, 44.95, 1.90) X(TheAur, 5, 59.7, 37.21, 2.65) X(IotAur, 4, 57.0, 33.17, 2.69) \
	X(EpsAur, 5, 2.0, 43.82, 3.03) X(EtaAur, 5, 6.5, 41.23, 3.18) \
	X(AlpGem, 7, 34.6, 31.89, 1.58) X(BetGem, 7, 45.3, 28.03, 1.16) X(GamGem, 6, 37.7, 16.40, 1.93) X(MuGem, 6, 23.0, 22.51, 2.87) \
	X(EpsGem, 6, 43.9, 25.13, 3.06) X(EtaGem, 6, 14.9, 22.51, 3.31) X(XiGem, 6, 45.3, 12.90, 3.35) X(DelGem, 7, 20.1, 21.98, 3.50) \
	X(AlpCMa, 6, 45.1, -16.72, -1.44) X(BetCMa, 6, 22.7, -17.96, 1.98) X(DelCMa, 7, 8.4, -26.39, 1.83) X(EpsCMa, 6, 58.6, -28.97, 1.50) \
	X(EtaCMa, 7, 24.1, -29.30, 2.45) X(ZetCMa, 6, 20.3, -30.06, 3.02) X(Omi2CMa, 7, 3.0, -23.83, 3.02) \
	X(AlpCMi, 7, 39.3, 5.22, .40) X(BetCMi, 7, 27.2, 8.29, 2.89) \
	X(AlpLep, 5, 32.7, -17.82, 2.58) X(BetLep, 5, 28.2, -20.76, 2.81) X(EpsLep, 5, 5.5, -22.37, 3.19) X(MuLep, 5, 12.9, -16.21, 3.29) \
	X(AlpCol, 5, 39.6, -34.07, 2.65) X(BetCol, 5, 51.0, -35.77, 3.12) \
	X(BetEri, 5, 7.8, -5.09, 2.78) X(GamEri, 3, 58.0, -13.51, 2.97) X(DelEri, 3, 43.2, -9.76, 3.52) X(TheEri, 2, 58.3, -40.30, 2.88) \
	X(ZetPup, 8, 3.6, -40.00, 2.21) X(PiPup, 7, 17.1, -37.10, 2.71) X(RhoPup, 8, 7.5, -24.30, 2.83) X(XiPup, 7, 49.3, -24.86, 3.34) \
	X(AlpHya, 9, 27.6, -8.66, 1.99) X(GamHya, 13, 18.9, -23.17, 2.99) X(ZetHya, 8, 55.4, 5.95, 3.11) X(NuHya, 10, 49.6, -16.19, 3.11) \
	X(PiHya, 14, 6.4, -26.68, 3.25) X(EpsHya, 8, 46.8, 6.42, 3.38) \
	X(AlpLeo, 10, 8.4, 11.97, 1.36) X(BetLeo, 11, 49.1, 14.57, 2.14) X(GamLeo, 10, 20.0, 19.84, 2.01) X(DelLeo, 11, 14.1, 20.52, 2.56) \
	X(EpsLeo, 9, 45.9, 23.77, 2.97) X(TheLeo, 11, 14.2, 15.43, 3.33) X(ZetLeo, 10, 16.7, 23.42, 3.43) X(EtaLeo, 10, 7.3, 16.76, 3.48) \
	X(MuLeo, 9, 52.8, 26.01, 3.88) \
	X(AlpUMa, 11, 3.7, 61.75, 1.81) X(BetUMa, 11, 1.8, 56.38, 2.34) X(GamUMa, 11, 53.8, 53.69, 2.41) X(DelUMa, 12, 15.4, 57.03, 3.32) \
	X(EpsUMa, 12, 54.0, 55.96, 1.76) X(ZetUMa, 13, 23.9, 54.93, 2.23) X(EtaUMa, 13, 47.5, 49.31, 1.85) X(PsiUMa, 11, 9.7, 44.50, 3.00) \
	X(MuUMa, 10, 22.3, 41.50, 3.06) X(IotUMa, 8, 59.2, 48.04, 3.12) X(TheUMa, 9, 32.9, 51.68, 3.17) X(OmiUMa, 8, 30.3, 60.72, 3.35) \
	X(LamUMa, 10, 17.1, 42.91, 3.45) X(NuUMa, 11, 18.5, 33.09, 3.49) \
	X(AlpUMi, 2, 31.8, 89.26, 1.97) X(BetUMi, 14, 50.7, 74.16, 2.07) X(GamUMi, 15, 20.7, 71.83, 3.00) \
	X(GamDra, 17, 56.6, 51.49, 2.24) X(BetDra, 17, 30.4, 52.30, 2.79) X(EtaDra, 16, 24.0, 61.51, 2.73) X(DelDra, 19, 12.6, 67.66, 3.07) \
	X(ZetDra, 17, 8.8, 65.71, 3.17) X(IotDra, 15, 24.9, 58.97, 3.29) X(AlpDra, 14, 4.4, 64.38, 3.67) \
	X(AlpCep, 21, 18.6, 62.59, 2.45) X(BetCep, 21, 28.7, 70.56, 3.23) X(GamCep, 23, 39.3, 77.63, 3.21) X(ZetCep, 22, 10.9, 58.20, 3.39) \
	X(EtaCep, 20, 45.3, 61.84, 3.41) X(IotCep, 22, 49.7, 66.20, 3.50) \
	X(AlpCas, 0, 40.5, 56.54, 2.24) X(BetCas, 0, 9.2, 59.15, 2.28) X(GamCas, 0, 56.7, 60.72, 2.15) X(DelCas, 1, 25.8, 60.24, 2.66) \
	X(EpsCas, 1, 54.4, 63.67, 3.35) X(EtaCas, 0, 49.1, 57.82, 3.46) \
	X(AlpPer, 3, 24.3, 49.86, 1.79) X(BetPer, 3, 8.2, 40.96, 2.09) X(ZetPer, 3, 54.1, 31.88, 2.84) X(EpsPer, 3, 57.9, 40.01, 2.90) \
	X(GamPer, 3, 4.8, 53.51, 2.91) X(DelPer, 3, 42.9, 47.79, 3.01) X(RhoPer, 3, 5.2, 38.84, 3.32) \
	X(AlpAnd, 0, 8.4, 29.09, 2.07) X(BetAnd, 1, 9.7, 35.62, 2.07) X(GamAnd, 2, 3.9, 42.33, 2.10) X(DelAnd, 0, 39.3, 30.86, 3.27) \
	X(AlpPeg, 23, 4.8, 15.21, 2.49) X(BetPeg, 23, 3.8, 28.08, 2.44) X(GamPeg, 0, 13.2, 15.18, 2.83) X(EpsPeg, 21, 44.2, 9.88, 2.38) \
	X(EtaPeg, 22, 43.0, 30.22, 2.93) X(ZetPeg, 22, 41.5, 10.83, 3.41) X(MuPeg, 22, 50.0, 24.60, 3.51) \
	X(AlpAri, 2, 7.2, 23.46, 2.01) X(BetAri, 1, 54.6, 20.81, 2.64) X(BetTri, 2, 9.5, 34.99, 3.00) \
	X(BetCet, 0, 43.6, -17.99, 2.04) X(AlpCet, 3, 2.3, 4.09, 2.54) X(EtaCet, 1, 8.6, -10.18, 3.46) X(GamCet, 2, 43.3, 3.24, 3.47) \
	X(TauCet, 1, 44.1, -15.94, 3.49) \
	X(BetAqr, 21, 31.6, -5.57, 2.90) X(AlpAqr, 22, 5.8, -.32, 2.95) X(DelAqr, 22, 54.7, -15.82, 3.27) \
	X(DelCap, 21, 47.0, -16.13, 2.85) X(BetCap, 20, 21.0, -14.78, 3.05) X(AlpCap, 20, 18.1, -12.54, 3.58) \
	X(AlpPsA, 22, 57.6, -29.62, 1.17) X(AlpGru, 22, 8.2, -46.96, 1.73) X(BetGru, 22, 42.7, -46.88, 2.07) \
	X(AlpCyg, 20, 41.4, 45.28, 1.25) X(GamCyg, 20, 22.2, 40.26, 2.23) X(EpsCyg, 20, 46.2, 33.97, 2.48) X(DelCyg, 19, 45.0, 45.13, 2.86) \
	X(BetCyg, 19, 30.7, 27.96, 3.05) X(ZetCyg, 21, 12.9, 30.23, 3.21) \
	X(AlpLyr, 18, 36.9, 38.78, .03) X(GamLyr, 18, 58.9, 32.69, 3.25) X(BetLyr, 18, 50.1, 33.36, 3.52) \
	X(AlpAql, 19, 50.8, 8.87, .76) X(GamAql, 19, 46.3, 10.61, 2.72) X(ZetAql, 19, 5.4, 13.86, 2.99) X(TheAql, 20, 11.3, -.82, 3.24) \
	X(DelAql, 19, 25.5, 3.11, 3.36) X(LamAql, 19, 6.2, -4.88, 3.43) X(BetAql, 19, 55.3, 6.41, 3.71) X(GamSge, 19, 58.8, 19.49, 3.51) \
	X(BetHer, 16, 30.2, 21.49, 2.78) X(ZetHer, 16, 41.3, 31.60, 2.81) X(DelHer, 17, 15.0, 24.84, 3.12) X(PiHer, 17, 15.0, 36.81, 3.16) \
	X(AlpHer, 17, 14.6, 14.39, 3.31) X(MuHer, 17, 46.5, 27.72, 3.42) X(EtaHer, 16, 42.9, 38.92, 3.48) X(EpsHer, 17, 0.3, 30.93, 3.92) \
	X(AlpOph, 17, 34.9, 12.56, 2.08) X(EtaOph, 17, 10.4, -15.72, 2.43) X(ZetOph, 16, 37.2, -10.57, 2.54) X(DelOph, 16, 14.3, -3.69, 2.73) \
	X(BetOph, 17, 43.5, 4.57, 2.76) X(KapOph, 16, 57.7, 9.38, 3.19) X(EpsOph, 16, 18.3, -4.69, 3.23) X(TheOph, 17, 22.0, -25.00, 3.27) \
	X(NuOph, 17, 59.0, -9.77, 3.32) \
	X(AlpSer, 15, 44.3, 6.43, 2.63) X(EtaSer, 18, 21.3, -2.90, 3.23) X(MuSer, 15, 49.6, -3.43, 3.54) \
	X(AlpCrB, 15, 34.7, 26.71, 2.22) X(BetCrB, 15, 27.8, 29.11, 3.66) X(GamCrB, 15, 42.7, 26.30, 3.81) X(TheCrB, 15, 32.9, 31.36, 4.14) \
	X(EpsCrB, 15, 57.6, 26.88, 4.14) X(DelCrB, 15, 49.6, 26.07, 4.59) \
	X(AlpBoo, 14, 15.7, 19.18, -.05) X(EpsBoo, 14, 45.0, 27.07, 2.35) X(EtaBoo, 13, 54.7, 18.40, 2.68) X(GamBoo, 14, 32.1, 38.31, 3.04) \
	X(BetBoo, 15, 1.9, 40.39, 3.49) X(DelBoo, 15, 15.5, 33.31, 3.46) X(AlpCVn, 12, 56.0, 38.32, 2.89) X(AlpLyn, 9, 21.1, 34.39, 3.14) \
	X(AlpVir, 13, 25.2, -11.16, .98) X(GamVir, 12, 41.7, -1.45, 2.74) X(EpsVir, 13, 2.2, 10.96, 2.85) X(ZetVir, 13, 34.7, -.60, 3.38) \
	X(DelVir, 12, 55.6, 3.40, 3.39) X(BetVir, 11, 50.7, 1.76, 3.59) X(EtaVir, 12, 19.9, -.67, 3.89) \
	X(GamCrv, 12, 15.8, -17.54, 2.58) X(BetCrv, 12, 34.4, -23.40, 2.65) X(DelCrv, 12, 29.9, -16.52, 2.94) X(EpsCrv, 12, 10.1, -22.62, 3.02) \
	X(BetLib, 15, 17.0, -9.38, 2.61) X(AlpLib, 14, 50.9, -16.04, 2.75) X(SigLib, 15, 4.1, -25.28, 3.25) \
	X(AlpSco, 16, 29.4, -26.43, 1.06) X(LamSco, 17, 33.6, -37.10, 1.62) X(TheSco, 17, 37.3, -43.00, 1.86) X(DelSco, 16, 0.3, -22.62, 2.29) \
	X(EpsSco, 16, 50.2, -34.29, 2.29) X(KapSco, 17, 42.5, -39.03, 2.39) X(BetSco, 16, 5.4, -19.81, 2.56) X(UpsSco, 17, 30.8, -37.30, 2.70) \
	X(TauSco, 16, 35.9, -28.22, 2.82) X(PiSco, 15, 58.9, -26.11, 2.89) X(SigSco, 16, 21.2, -25.59, 2.90) X(IotSco, 17, 47.6, -40.13, 2.99) \
	X(MuSco, 16, 51.9, -38.05, 3.00) X(EtaSco, 17, 12.2, -43.24, 3.32) X(ZetSco, 16, 54.6, -42.36, 3.62) \
	X(EpsSgr, 18, 24.2, -34.38, 1.79) X(SigSgr, 18, 55.3, -26.30, 2.05) X(ZetSgr, 19, 2.6, -29.88, 2.60) X(DelSgr, 18, 21.0, -29.83, 2.72) \
	X(LamSgr, 18, 28.0, -25.42, 2.82) X(PiSgr, 19, 9.8, -21.02, 2.88) X(GamSgr, 18, 5.8, -30.42, 2.98) X(EtaSgr, 18, 17.6, -36.76, 3.10) \
	X(PhiSgr, 18, 45.7, -26.99, 3.17) X(TauSgr, 19, 6.9, -27.67, 3.32) \
	X(AlpLup, 14, 41.9, -47.39, 2.30) X(BetLup, 14, 58.5, -43.13, 2.68) X(GamLup, 15, 35.1, -41.17, 2.80) \
	X(TheCen, 14, 6.7, -36.37, 2.06) X(IotCen, 13, 20.6, -36.71, 2.75) X(EtaCen, 14, 35.5, -42.16, 2.33) X(ZetCen, 13, 55.5, -47.29, 2.55) \
	X(GamCen, 12, 41.5, -48.96, 2.20) X(LamVel, 9, 8.0, -43.43, 2.23) X(GamVel, 8, 9.5, -47.34, 1.75) X(AlpPhe, 0, 26.3, -42.31, 2.40)

namespace S47SkySphere
{
	constexpr double LongitudeWest = 106.6;  // the valley's, as the game's own moon and sun have it (S47Weather)
	constexpr double UtcBehindHours = 7.;    // Mountain Standard Time behind Greenwich
	constexpr double EpochYear = 1986.33;    // the game's spring: the sky "of date"
	constexpr double Persp = 2.5;            // the picture's point of projection, sphere radii behind its centre (a gentle perspective)
	constexpr double RimMargin = .955;       // the array's window stays this far inside the rim
	constexpr bool bZenithUp = false;        // false: celestial north up, as an equatorial chart; true: the zenith up

	namespace Star
	{
		enum EId : int32
		{
#define S47_SKY_STAR_ID(Name, RaH, RaM, DecDeg, Mag) Name,
			S47_SKY_STAR_TABLE(S47_SKY_STAR_ID)
#undef S47_SKY_STAR_ID
			Count
		};
	}
	struct FStar { float RaHours, Dec, Mag; };
	inline constexpr FStar Stars[] =
	{
#define S47_SKY_STAR_ROW(Name, RaH, RaM, DecDeg, Mag) { float((RaH) + (RaM) / 60.), float(DecDeg), float(Mag) },
		S47_SKY_STAR_TABLE(S47_SKY_STAR_ROW)
#undef S47_SKY_STAR_ROW
	};
	// The stick figures: lines between stars of the table.
	struct FLink { int32 A, B; };
	inline constexpr FLink Links[] =
	{
		// Orion
		{ Star::AlpOri, Star::GamOri }, { Star::AlpOri, Star::ZetOri }, { Star::GamOri, Star::DelOri }, { Star::DelOri, Star::EpsOri }, { Star::EpsOri, Star::ZetOri },
		{ Star::ZetOri, Star::KapOri }, { Star::DelOri, Star::BetOri }, { Star::AlpOri, Star::LamOri }, { Star::GamOri, Star::LamOri },
		// Taurus
		{ Star::AlpTau, Star::GamTau }, { Star::GamTau, Star::EpsTau }, { Star::AlpTau, Star::ZetTau }, { Star::EpsTau, Star::BetTau }, { Star::GamTau, Star::LamTau },
		// Auriga
		{ Star::AlpAur, Star::BetAur }, { Star::BetAur, Star::TheAur }, { Star::TheAur, Star::BetTau }, { Star::BetTau, Star::IotAur }, { Star::IotAur, Star::AlpAur },
		// Gemini
		{ Star::AlpGem, Star::BetGem }, { Star::AlpGem, Star::EpsGem }, { Star::EpsGem, Star::MuGem }, { Star::MuGem, Star::EtaGem }, { Star::BetGem, Star::DelGem }, { Star::DelGem, Star::GamGem },
		// Canis Major and Minor
		{ Star::AlpCMa, Star::BetCMa }, { Star::AlpCMa, Star::Omi2CMa }, { Star::Omi2CMa, Star::DelCMa }, { Star::DelCMa, Star::EpsCMa }, { Star::DelCMa, Star::EtaCMa }, { Star::AlpCMi, Star::BetCMi },
		// Leo
		{ Star::AlpLeo, Star::EtaLeo }, { Star::EtaLeo, Star::GamLeo }, { Star::GamLeo, Star::ZetLeo }, { Star::ZetLeo, Star::MuLeo }, { Star::MuLeo, Star::EpsLeo },
		{ Star::AlpLeo, Star::TheLeo }, { Star::TheLeo, Star::BetLeo }, { Star::BetLeo, Star::DelLeo }, { Star::DelLeo, Star::GamLeo }, { Star::DelLeo, Star::TheLeo },
		// Ursa Major (the dipper) and the guards of the pole
		{ Star::AlpUMa, Star::BetUMa }, { Star::BetUMa, Star::GamUMa }, { Star::GamUMa, Star::DelUMa }, { Star::DelUMa, Star::AlpUMa },
		{ Star::DelUMa, Star::EpsUMa }, { Star::EpsUMa, Star::ZetUMa }, { Star::ZetUMa, Star::EtaUMa }, { Star::BetUMi, Star::GamUMi },
		// Cassiopeia and Cepheus
		{ Star::BetCas, Star::AlpCas }, { Star::AlpCas, Star::GamCas }, { Star::GamCas, Star::DelCas }, { Star::DelCas, Star::EpsCas },
		{ Star::AlpCep, Star::BetCep }, { Star::BetCep, Star::IotCep }, { Star::IotCep, Star::ZetCep }, { Star::ZetCep, Star::AlpCep }, { Star::BetCep, Star::GamCep }, { Star::GamCep, Star::IotCep },
		// Perseus, Andromeda, Pegasus, Aries
		{ Star::GamPer, Star::AlpPer }, { Star::AlpPer, Star::DelPer }, { Star::DelPer, Star::EpsPer }, { Star::EpsPer, Star::ZetPer }, { Star::AlpPer, Star::BetPer }, { Star::BetPer, Star::RhoPer },
		{ Star::AlpAnd, Star::DelAnd }, { Star::DelAnd, Star::BetAnd }, { Star::BetAnd, Star::GamAnd },
		{ Star::AlpPeg, Star::BetPeg }, { Star::BetPeg, Star::AlpAnd }, { Star::AlpAnd, Star::GamPeg }, { Star::GamPeg, Star::AlpPeg },
		{ Star::AlpPeg, Star::ZetPeg }, { Star::ZetPeg, Star::EpsPeg }, { Star::BetPeg, Star::EtaPeg }, { Star::BetPeg, Star::MuPeg }, { Star::AlpAri, Star::BetAri },
		// Cygnus, Lyra, Aquila
		{ Star::AlpCyg, Star::GamCyg }, { Star::GamCyg, Star::BetCyg }, { Star::DelCyg, Star::GamCyg }, { Star::GamCyg, Star::EpsCyg }, { Star::EpsCyg, Star::ZetCyg },
		{ Star::AlpLyr, Star::BetLyr }, { Star::BetLyr, Star::GamLyr }, { Star::GamLyr, Star::AlpLyr },
		{ Star::GamAql, Star::AlpAql }, { Star::AlpAql, Star::BetAql }, { Star::AlpAql, Star::DelAql }, { Star::DelAql, Star::LamAql }, { Star::ZetAql, Star::DelAql }, { Star::DelAql, Star::TheAql },
		// Hercules (the keystone) and Ophiuchus
		{ Star::ZetHer, Star::EtaHer }, { Star::EtaHer, Star::PiHer }, { Star::PiHer, Star::EpsHer }, { Star::EpsHer, Star::ZetHer }, { Star::ZetHer, Star::BetHer }, { Star::EpsHer, Star::DelHer },
		{ Star::AlpOph, Star::BetOph }, { Star::BetOph, Star::EtaOph }, { Star::EtaOph, Star::ZetOph }, { Star::ZetOph, Star::DelOph }, { Star::DelOph, Star::KapOph }, { Star::KapOph, Star::AlpOph },
		// Corona Borealis, Bootes, Virgo, Corvus, Libra
		{ Star::TheCrB, Star::BetCrB }, { Star::BetCrB, Star::AlpCrB }, { Star::AlpCrB, Star::GamCrB }, { Star::GamCrB, Star::DelCrB }, { Star::DelCrB, Star::EpsCrB },
		{ Star::AlpBoo, Star::EpsBoo }, { Star::EpsBoo, Star::DelBoo }, { Star::DelBoo, Star::BetBoo }, { Star::BetBoo, Star::GamBoo }, { Star::GamBoo, Star::AlpBoo }, { Star::AlpBoo, Star::EtaBoo },
		{ Star::AlpVir, Star::GamVir }, { Star::GamVir, Star::DelVir }, { Star::DelVir, Star::EpsVir }, { Star::GamVir, Star::EtaVir }, { Star::EtaVir, Star::BetVir }, { Star::AlpVir, Star::ZetVir },
		{ Star::GamCrv, Star::EpsCrv }, { Star::EpsCrv, Star::BetCrv }, { Star::BetCrv, Star::DelCrv }, { Star::DelCrv, Star::GamCrv },
		{ Star::AlpLib, Star::BetLib }, { Star::AlpLib, Star::SigLib },
		// Scorpius and Sagittarius (the teapot)
		{ Star::BetSco, Star::DelSco }, { Star::DelSco, Star::PiSco }, { Star::DelSco, Star::SigSco }, { Star::SigSco, Star::AlpSco }, { Star::AlpSco, Star::TauSco },
		{ Star::TauSco, Star::EpsSco }, { Star::EpsSco, Star::MuSco }, { Star::MuSco, Star::ZetSco }, { Star::ZetSco, Star::EtaSco }, { Star::EtaSco, Star::TheSco },
		{ Star::TheSco, Star::IotSco }, { Star::IotSco, Star::KapSco }, { Star::KapSco, Star::LamSco }, { Star::LamSco, Star::UpsSco },
		{ Star::DelSgr, Star::EpsSgr }, { Star::EpsSgr, Star::ZetSgr }, { Star::ZetSgr, Star::PhiSgr }, { Star::PhiSgr, Star::DelSgr }, { Star::DelSgr, Star::LamSgr }, { Star::LamSgr, Star::PhiSgr },
		{ Star::DelSgr, Star::GamSgr }, { Star::GamSgr, Star::EpsSgr }, { Star::PhiSgr, Star::SigSgr }, { Star::SigSgr, Star::TauSgr }, { Star::TauSgr, Star::ZetSgr },
		// Lepus
		{ Star::AlpLep, Star::BetLep }, { Star::AlpLep, Star::MuLep }, { Star::BetLep, Star::EpsLep },
	};
	// Three of the card sources where the sky has them (J2000), for the self-test: the cards were written from the real sky of
	// 13 April 1986 (FS47Observing::Runs), so the terminal's clock and the cards have to agree.
	struct FKnown { const TCHAR* Name; double RaDeg, DecDeg, ClockHours; int32 Run; };
	inline constexpr FKnown Known[] =
	{
		{ TEXT("3C 286"), 202.78, 30.51, 19., 0 }, { TEXT("PSR B1133+16"), 174.01, 15.85, 19. + 50. / 60., 1 }, { TEXT("Virgo A"), 187.71, 12.39, 22. + 5. / 60., 3 },
	};

	inline double Wrap360(double Deg) { Deg = FMath::Fmod(Deg, 360.); return Deg < 0 ? Deg + 360. : Deg; }
	inline double Wrap180(double Deg) { return Wrap360(Deg + 180.) - 180.; }
	inline double Hash01(uint32 V) { V ^= V >> 16; V *= 0x7feb352du; V ^= V >> 15; V *= 0x846ca68bu; V ^= V >> 16; return double(V & 0xffffffu) / double(0x1000000); }
	// The unit vector of a longitude and a latitude (degrees), and back.
	inline FVector Dir(double LonDeg, double LatDeg)
	{
		const double L = FMath::DegreesToRadians(LonDeg), B = FMath::DegreesToRadians(LatDeg);
		return FVector(FMath::Cos(B) * FMath::Cos(L), FMath::Cos(B) * FMath::Sin(L), FMath::Sin(B));
	}
	inline void LonLat(const FVector& V, double& LonDeg, double& LatDeg)
	{
		const double Len = FMath::Max(V.Size(), 1e-12);
		LonDeg = Wrap360(FMath::RadiansToDegrees(FMath::Atan2(V.Y, V.X)));
		LatDeg = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(V.Z / Len, -1., 1.)));
	}

	// ---- the ground and the sky ----
	// Azimuth (from north through east) and elevation to hour angle (degrees, west positive) and declination, at the station's
	// latitude, and back. (FS47SkyAcquire::HourAngleDec is the same rule in hours.)
	inline void AzElToHaDec(const FVector2D& AzEl, double& HaDeg, double& DecDeg)
	{
		const double A = FMath::DegreesToRadians(AzEl.X), E = FMath::DegreesToRadians(AzEl.Y), Phi = FMath::DegreesToRadians(FS47SkyAcquire::Latitude);
		DecDeg = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(FMath::Sin(Phi) * FMath::Sin(E) + FMath::Cos(Phi) * FMath::Cos(E) * FMath::Cos(A), -1., 1.)));
		HaDeg = FMath::RadiansToDegrees(FMath::Atan2(-FMath::Sin(A) * FMath::Cos(E), FMath::Sin(E) * FMath::Cos(Phi) - FMath::Cos(E) * FMath::Sin(Phi) * FMath::Cos(A)));
	}
	inline FVector2D HaDecToAzEl(double HaDeg, double DecDeg)
	{
		const double H = FMath::DegreesToRadians(HaDeg), D = FMath::DegreesToRadians(DecDeg), Phi = FMath::DegreesToRadians(FS47SkyAcquire::Latitude);
		const double El = FMath::Asin(FMath::Clamp(FMath::Sin(Phi) * FMath::Sin(D) + FMath::Cos(Phi) * FMath::Cos(D) * FMath::Cos(H), -1., 1.));
		const double Az = FMath::Atan2(-FMath::Sin(H) * FMath::Cos(D), FMath::Sin(D) * FMath::Cos(Phi) - FMath::Cos(D) * FMath::Sin(Phi) * FMath::Cos(H));
		return FVector2D(Wrap360(FMath::RadiansToDegrees(Az)), FMath::RadiansToDegrees(El));
	}
	// The Julian day of a calendar date at 0 h, Greenwich's mean sidereal time for a Julian day, and the station's own.
	inline double JulianDay(int32 Year, int32 Month, int32 Day)
	{
		const int32 A = (14 - Month) / 12, Y = Year + 4800 - A, Mo = Month + 12 * A - 3;
		return double(Day + (153 * Mo + 2) / 5 + 365 * Y + Y / 4 - Y / 100 + Y / 400 - 32045) - .5;
	}
	inline double GmstDeg(double JulianDayUt) { return Wrap360(280.46061837 + 360.98564736629 * (JulianDayUt - 2451545.)); }
	inline double LstDeg(double JulianDayUt) { return Wrap360(GmstDeg(JulianDayUt) - LongitudeWest); }
	// The station's sidereal time for a date and its Mountain clock (seconds; may run past midnight).
	inline double LstOf(int32 Year, int32 Month, int32 Day, double ClockSeconds) { return LstDeg(JulianDay(Year, Month, Day) + (ClockSeconds + UtcBehindHours * 3600.) / 86400.); }
	// A place of date and the sidereal time give the ground's azimuth and elevation, and back.
	inline FVector2D RaDecToAzEl(double RaDeg, double DecDeg, double Lst) { return HaDecToAzEl(Wrap180(Lst - RaDeg), DecDeg); }
	inline void AzElToRaDec(const FVector2D& AzEl, double Lst, double& RaDeg, double& DecDeg)
	{
		double Ha = 0;
		AzElToHaDec(AzEl, Ha, DecDeg);
		RaDeg = Wrap360(Lst - Ha);
	}
	// Precession over a span of years, to first order (the annual rates m and n): good to a hundredth of a degree over fifty years
	// away from the poles, which is finer than this picture.
	inline void Precess(double& RaDeg, double& DecDeg, double Years)
	{
		constexpr double RateM = 3.07496 / 240., RateN = 1.33621 / 240.; // degrees a year
		const double A = FMath::DegreesToRadians(RaDeg), D = FMath::DegreesToRadians(FMath::Clamp(DecDeg, -88., 88.));
		RaDeg = Wrap360(RaDeg + Years * (RateM + RateN * FMath::Sin(A) * FMath::Tan(D)));
		DecDeg = FMath::Clamp(DecDeg + Years * RateN * FMath::Cos(A), -90., 90.);
	}
	// A place of date as the catalogues of 1986 print it (B1950.0), and as J2000.
	inline void ToB1950(double& RaDeg, double& DecDeg) { Precess(RaDeg, DecDeg, 1950. - EpochYear); }
	inline void ToJ2000(double& RaDeg, double& DecDeg) { Precess(RaDeg, DecDeg, 2000. - EpochYear); }
	inline void FromJ2000(double& RaDeg, double& DecDeg) { Precess(RaDeg, DecDeg, EpochYear - 2000.); }
	// Galactic longitude and latitude of a J2000 place, and back (the rotation the Hipparcos catalogue gives).
	inline void EqToGal(double RaDeg, double DecDeg, double& LonDeg, double& LatDeg)
	{
		const FVector E = Dir(RaDeg, DecDeg);
		LonLat(FVector(-.0548755604 * E.X - .8734370902 * E.Y - .4838350155 * E.Z, .4941094279 * E.X - .4448296300 * E.Y + .7469822445 * E.Z, -.8676661490 * E.X - .1980763734 * E.Y + .4559837762 * E.Z), LonDeg, LatDeg);
	}
	inline void GalToEq(double LonDeg, double LatDeg, double& RaDeg, double& DecDeg)
	{
		const FVector G = Dir(LonDeg, LatDeg);
		LonLat(FVector(-.0548755604 * G.X + .4941094279 * G.Y - .8676661490 * G.Z, -.8734370902 * G.X - .4448296300 * G.Y - .1980763734 * G.Z, -.4838350155 * G.X + .7469822445 * G.Y + .4559837762 * G.Z), RaDeg, DecDeg);
	}

	// ---- the galactic plane as a stippled strip: the same specks every night, more of them and a wider band toward the centre ----
	constexpr int32 DustCount = 300;
	struct FDust { float Lon, Lat, Bright; };
	inline FDust Dust(int32 Index)
	{
		const uint32 K = uint32(Index) * 2654435761u + 0x47u;
		const double T = 2. * Hash01(K) - 1., Spread = Hash01(K ^ 0x9e3779b9u) + Hash01(K ^ 0x85ebca6bu) - 1.; // -1..1, most of it near 0
		const double Lon = 180. * (T - .28 * FMath::Sin(UE_DOUBLE_PI * T));
		const double Toward = FMath::Square(FMath::Cos(FMath::DegreesToRadians(Lon) * .5)); // 1 at the centre, 0 opposite it
		FDust D;
		D.Lon = float(Wrap360(Lon)); D.Lat = float(Spread * (3.5 + 8.5 * Toward)); D.Bright = float((.4 + .6 * Toward) * (1. - .7 * FMath::Abs(Spread)));
		return D;
	}

	// ---- the picture ----
	// A gentle perspective of the sphere seen from inside, about the middle of the array's window, shown through an ellipse (the
	// rim). With celestial north up it is drawn wider than high, so that the sphere fills the rim. Centre, Right and Up are
	// directions in the hour-angle frame (x to the meridian on the equator, y to the west point, z to the pole): they do not turn
	// with the sky.
	struct FView
	{
		FVector Centre = FVector(1, 0, 0), Right = FVector(0, 1, 0), Up = FVector(0, 0, 1);
		FVector2D Mid = FVector2D::ZeroVector;                                // where the centre stands on the glass
		FVector2D Rim = FVector2D::ZeroVector, Semi = FVector2D(100, 100);    // the rim: its middle, half its width and height
		double Radius = 100.;  // units a radian at the centre, up the glass
		double Stretch = 1.;   // how much wider across
	};
	inline bool Project(const FView& V, const FVector& S, FVector2D& Out)
	{
		const double Cd = FVector::DotProduct(S, V.Centre);
		if (Cd < -.25) return false; // behind the picture
		const double K = (Persp + 1.) / (Persp + Cd) * V.Radius;
		Out = FVector2D(V.Mid.X + FVector::DotProduct(S, V.Right) * K * V.Stretch, V.Mid.Y - FVector::DotProduct(S, V.Up) * K);
		return true;
	}
	inline FVector Unproject(const FView& V, const FVector2D& P)
	{
		const double X = (P.X - V.Mid.X) / FMath::Max(V.Radius * V.Stretch, 1e-6), Y = -(P.Y - V.Mid.Y) / FMath::Max(V.Radius, 1e-6);
		const double Rho = FMath::Sqrt(X * X + Y * Y);
		if (Rho < 1e-9) return V.Centre;
		const double K = Persp + 1.;
		const double Angle = FMath::Asin(FMath::Clamp(Rho * Persp / FMath::Sqrt(K * K + Rho * Rho), -1., 1.)) + FMath::Atan2(Rho, K);
		return V.Centre * FMath::Cos(Angle) + (V.Right * (X / Rho) + V.Up * (Y / Rho)) * FMath::Sin(Angle);
	}
	inline bool ProjectHaDec(const FView& V, double HaDeg, double DecDeg, FVector2D& Out) { return Project(V, Dir(HaDeg, DecDeg), Out); }
	inline bool ProjectAzEl(const FView& V, const FVector2D& AzEl, FVector2D& Out)
	{
		double Ha = 0, Dec = 0;
		AzElToHaDec(AzEl, Ha, Dec);
		return Project(V, Dir(Ha, Dec), Out);
	}
	inline FVector2D UnprojectAzEl(const FView& V, const FVector2D& P)
	{
		double Ha = 0, Dec = 0;
		LonLat(Unproject(V, P), Ha, Dec);
		return HaDecToAzEl(Ha, Dec);
	}
	// Inside the rim (Shrink below 1: this far inside it).
	inline bool Inside(const FView& V, const FVector2D& P, double Shrink = 1.)
	{
		const double X = (P.X - V.Rim.X) / FMath::Max(V.Semi.X * Shrink, 1e-6), Y = (P.Y - V.Rim.Y) / FMath::Max(V.Semi.Y * Shrink, 1e-6);
		return X * X + Y * Y <= 1.;
	}
	// A segment cut to the rim; false when none of it is inside. bCutA, bCutB: that end was moved onto the rim.
	inline bool CutToRim(const FView& V, FVector2D& A, FVector2D& B, bool& bCutA, bool& bCutB)
	{
		bCutA = bCutB = false;
		const FVector2D Inv(1. / FMath::Max(V.Semi.X, 1e-6), 1. / FMath::Max(V.Semi.Y, 1e-6));
		const FVector2D P0((A.X - V.Rim.X) * Inv.X, (A.Y - V.Rim.Y) * Inv.Y), D((B.X - A.X) * Inv.X, (B.Y - A.Y) * Inv.Y);
		const double Qa = D.SizeSquared(), Qb = 2. * FVector2D::DotProduct(P0, D), Qc = P0.SizeSquared() - 1.;
		if (Qa < 1e-18) return Qc <= 0;
		const double Disc = Qb * Qb - 4. * Qa * Qc;
		if (Disc <= 0) return false;
		const double Root = FMath::Sqrt(Disc), T0 = (-Qb - Root) / (2. * Qa), T1 = (-Qb + Root) / (2. * Qa);
		if (T1 <= 0 || T0 >= 1) return false;
		const FVector2D From = A, Step = B - A;
		if (T0 > 0) { A = From + Step * T0; bCutA = true; }
		if (T1 < 1) { B = From + Step * T1; bCutB = true; }
		return true;
	}
	// The picture for a rim on the glass: about the middle of the array's window, placed and sized so that every edge of the
	// window stays RimMargin inside the rim (the smallest ellipse of the rim's shape round the window's edge, found by walking
	// its middle toward the farthest point).
	inline FView FitView(const FVector2D& RimMid, const FVector2D& RimSemi)
	{
		using FSky = FS47SkyAcquire;
		FView V;
		V.Rim = RimMid; V.Semi = FVector2D(FMath::Max(RimSemi.X, 8.), FMath::Max(RimSemi.Y, 8.));
		double Ha = 0, Dec = 0;
		AzElToHaDec(FVector2D((FSky::AzMin + FSky::AzMax) * .5, (FSky::ElMin + FSky::ElMax) * .5), Ha, Dec);
		const double H = FMath::DegreesToRadians(Ha), D = FMath::DegreesToRadians(Dec);
		V.Centre = Dir(Ha, Dec);
		const FVector West(-FMath::Sin(H), FMath::Cos(H), 0.), North(-FMath::Sin(D) * FMath::Cos(H), -FMath::Sin(D) * FMath::Sin(H), FMath::Cos(D));
		V.Right = West; V.Up = North;
		double Wider = V.Semi.X / V.Semi.Y;
		if (bZenithUp)
		{
			const FVector Zenith = Dir(0., FSky::Latitude);
			const FVector Lift = (Zenith - V.Centre * FVector::DotProduct(Zenith, V.Centre)).GetSafeNormal();
			if (!Lift.IsNearlyZero()) { V.Up = Lift; V.Right = FVector::CrossProduct(Lift, V.Centre); Wider = 1.; } // (right x up is the centre, as west x north is)
		}
		// the window's edge with the radius at one unit, in the rim's own measure (half its width and half its height are 1)
		V.Radius = 1.; V.Stretch = 1.; V.Mid = FVector2D::ZeroVector;
		TArray<FVector2D, TInlineAllocator<192>> Edge;
		auto Add = [&](double Az, double El) { FVector2D P; if (ProjectAzEl(V, FVector2D(Az, El), P)) Edge.Add(FVector2D(P.X * Wider / V.Semi.X, P.Y / V.Semi.Y)); };
		for (double Az = FSky::AzMin; Az <= FSky::AzMax + 1e-9; Az += 2.) { Add(Az, FSky::ElMin); Add(Az, FSky::ElMax); }
		for (double El = FSky::ElMin; El <= FSky::ElMax + 1e-9; El += 2.) { Add(FSky::AzMin, El); Add(FSky::AzMax, El); }
		FVector2D Middle = FVector2D::ZeroVector;
		double Reach = 1e-9;
		for (int32 Pass = 1; Pass <= 48; ++Pass)
		{
			FVector2D Far = Middle; double Most = -1.;
			for (const FVector2D& P : Edge) { const double Away = FVector2D::DistSquared(P, Middle); if (Away > Most) { Most = Away; Far = P; } }
			Middle += (Far - Middle) / double(Pass + 1);
		}
		for (const FVector2D& P : Edge) Reach = FMath::Max(Reach, FVector2D::Distance(P, Middle));
		V.Radius = RimMargin / Reach; V.Stretch = Wider;
		V.Mid = FVector2D(V.Rim.X - Middle.X * V.Semi.X * V.Radius, V.Rim.Y - Middle.Y * V.Semi.Y * V.Radius);
		return V;
	}
	// The direction in azimuth and elevation (a unit vector, plain degrees as the rule counts them) that moves a place the given way
	// on the glass (x to the right, y up). The steering keys move the reticle the way they point on the picture.
	inline FVector2D SteerOnGlass(const FView& V, const FVector2D& AzEl, const FVector2D& GlassDir)
	{
		if (GlassDir.IsNearlyZero()) return FVector2D::ZeroVector;
		constexpr double Eps = .05;
		FVector2D P0, Pa, Pe;
		if (!ProjectAzEl(V, AzEl, P0) || !ProjectAzEl(V, AzEl + FVector2D(Eps, 0), Pa) || !ProjectAzEl(V, AzEl + FVector2D(0, Eps), Pe)) return GlassDir.GetSafeNormal();
		// columns: what a step of azimuth and a step of elevation do on the glass (y up)
		const FVector2D Ja((Pa.X - P0.X) / Eps, -(Pa.Y - P0.Y) / Eps), Je((Pe.X - P0.X) / Eps, -(Pe.Y - P0.Y) / Eps);
		const double Det = Ja.X * Je.Y - Je.X * Ja.Y;
		if (FMath::Abs(Det) < 1e-9) return GlassDir.GetSafeNormal();
		const FVector2D Step((GlassDir.X * Je.Y - Je.X * GlassDir.Y) / Det, (Ja.X * GlassDir.Y - GlassDir.X * Ja.Y) / Det);
		return Step.GetSafeNormal();
	}
	// How many units on the glass a step of one of the rule's degrees is at a place, along a direction in azimuth and elevation.
	inline double UnitsPerDegree(const FView& V, const FVector2D& AzEl, const FVector2D& AzElDir)
	{
		constexpr double Eps = .05;
		const FVector2D Unit = AzElDir.GetSafeNormal();
		FVector2D P0, P1;
		if (!ProjectAzEl(V, AzEl, P0) || !ProjectAzEl(V, AzEl + Unit * Eps, P1)) return 0.;
		return FVector2D::Distance(P0, P1) / Eps;
	}

	// ---- the readouts' words ----
	inline FString RaText(double RaDeg)
	{
		const int32 Total = FMath::RoundToInt32(Wrap360(RaDeg) / 15. * 3600.) % 86400;
		return FString::Printf(TEXT("%02dh%02dm%02ds"), Total / 3600, Total / 60 % 60, Total % 60);
	}
	inline FString DecText(double DecDeg)
	{
		const int32 Total = FMath::RoundToInt32(FMath::Abs(DecDeg) * 60.);
		return FString::Printf(TEXT("%s%02d\u00b0%02d'"), DecDeg < 0 ? TEXT("-") : TEXT("+"), Total / 60, Total % 60);
	}

	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}

#include "S47Flora.h"

#include "Algo/StableSort.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/PlatformTime.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47Three.h"
#include "S47Web.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Flora, Log, All);

// ---------- the kinds ----------
// One kind: how many per square metre in the open desert, how much more cover can add, how far it is
// drawn, how much of it Low keeps, how much of it grows where the player walks and how big at most
// there, the sizes, and the size of its patches; cap: copies in the mesh at most; clump: the size of
// its drifts; nurse: the share of it that stands round a shrub of its cell; seam: how much of it
// grows in a way's seams; far: past this distance only this share of it is drawn.
struct FS47Flora::FKind
{
	ES47FloraKind Id; double Rate, Most, Reach, Low, Walk, WalkMax; double Size[2]; double Patch; int32 Cap;
	double Clump, Nurse, Seam; double Far[2]; bool bFar;
};

namespace
{
	using FKind = FS47Flora::FKind;
	const FKind KINDS[] = {
		{ ES47FloraKind::Grass, 0.46, 3.8, 65, 0.45, 0.85, 1.0, { 0.3, 1.5 }, 26, 9000, 3.5, 0.55, 3.6, { 0, 0 }, false },
		{ ES47FloraKind::Sage, 0.028, 2.2, 110, 0.7, 0.35, 0.5, { 0.45, 1.35 }, 38, 1600, 0, 0, 0, { 0, 0 }, false },
		{ ES47FloraKind::Creosote, 0.006, 1.4, 240, 0.8, 0, 0, { 0.7, 1.4 }, 60, 600, 0, 0, 0, { 110, 0.14 }, true },
		{ ES47FloraKind::Stalks, 0.012, 3.4, 70, 0.4, 0.5, 0.75, { 0.6, 1.3 }, 30, 1500, 4, 0.65, 0, { 0, 0 }, false },
		// yucca and cholla in one mesh (a mirrored copy is a cholla: succulentShape)
		{ ES47FloraKind::Succulent, 0.0027, 1.1, 260, 1, 0, 0, { 0.62, 1.28 }, 60, 520, 0, 0, 0, { 130, 0.14 }, true },
		{ ES47FloraKind::Stones, 0.07, 1.6, 36, 0.35, 0.8, 0.9, { 0.3, 2.1 }, 18, 3000, 0, 0, 1.4, { 0, 0 }, false },
		// the stains in the dirt, and the litter under a big tuft or a shrub (ranked last: near only)
		{ ES47FloraKind::Stain, 0.0045, 1.2, 90, 0, 1, 1.4, { 0.9, 2.6 }, 1, 1100, 0, 0, 0, { 42, 0.5 }, true },
		// (Freeform's rocks only: nothing of it in the web version's rules; Ahead and Fill pass it by)
		{ ES47FloraKind::Rock, 0, 0, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, { 0, 0 }, false },
		// (Freeform's desert plants only, the same)
		{ ES47FloraKind::Yucca, 0, 0, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, { 0, 0 }, false },
		{ ES47FloraKind::Cholla, 0, 0, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, { 0, 0 }, false },
		{ ES47FloraKind::Mesquite, 0, 0, 0, 0, 0, 0, { 0, 0 }, 0, 0, 0, 0, 0, { 0, 0 }, false },
	};
	// Freeform's rules of nature (FS47FloraSpec::bNatural), measured by eye from the references
	// of southern New Mexico (7 Nov 2026): creosote in dense stands 1-3 m apart with wide, nearly bare
	// fields between (NoiseAt: Stand; Make: a jittered grid, nine to a cell); grass in small tufts, drifts
	// and bands in its own wide meadows; stones in clusters, small in the middle distance (Make), a few
	// rocks of 0.3-0.6 m (Rock: Sandstone_A); no stain of its own: a soft dark foot under each shrub.
	// What differs from KINDS: grass (size, count, nursing), sage (rate, reach, far: the carpet out to about
	// 200 m), creosote (grid, size, cap, far), stalks (rate, nursing), stones (rate, reach), stain (reach, far), rock.
	// Tried outside the engine with these numbers (open desert, three places; 7 Nov 2026): grass 490-1080 before,
	// 1340-1480 now; sage 210-410, now 1560-2170 (1110-1380 of them past 70 m); stalks about 100, now about 260;
	// creosote 1140-1400 at its old cap, now 1220-2000; the dead ones 190-370 more (Mesquite's cap).
	const FKind NATURAL[] = {
		{ ES47FloraKind::Grass, 0.46, 3.8, 65, 0.45, 0.85, 1.0, { 0.3, 0.8 }, 26, 5200, 3.5, 0.5, 3.6, { 0, 0 }, false },
		{ ES47FloraKind::Sage, 0.045, 2.2, 235, 0.7, 0.35, 0.5, { 0.45, 1.2 }, 38, 2600, 0, 0, 0, { 90, 0.45 }, true },
		{ ES47FloraKind::Creosote, 0.0625, 1.0, 240, 0.8, 0, 0, { 0.5, 1.3 }, 60, 2000, 0, 0, 0, { 55, 0.3 }, true },
		{ ES47FloraKind::Stalks, 0.03, 3.4, 70, 0.4, 0.5, 0.75, { 0.6, 1.3 }, 30, 1500, 4, 0.45, 0, { 0, 0 }, false },
		{ ES47FloraKind::Succulent, 0.0027, 1.1, 260, 1, 0, 0, { 0.62, 1.28 }, 60, 520, 0, 0, 0, { 130, 0.14 }, true },
		{ ES47FloraKind::Stones, 0.14, 1.6, 90, 0.35, 0.8, 0.9, { 0.3, 2.1 }, 18, 3000, 0, 0, 1.4, { 0, 0 }, false },
		{ ES47FloraKind::Stain, 0.0045, 1.2, 110, 0, 1, 1.4, { 0.9, 2.6 }, 1, 1100, 0, 0, 0, { 70, 0.5 }, true },
		{ ES47FloraKind::Rock, 0.005, 1.0, 80, 0.5, 0, 0, { 0.3, 0.6 }, 1, 240, 0, 0, 0, { 0, 0 }, false },
		// the desert plants (estimates from the models' sizes, not measured in the game); the sizes are the variants' (DESERT_PARTS).
		// Most 2.2: the density is still Rate * D, but no longer clipped at 1.1, so the patches, the areas' mix and the farm's FarmMul tell
		{ ES47FloraKind::Yucca, 0.0016, 2.2, 260, 1, 0, 0, { 0.72, 1.3 }, 60, 320, 0, 0, 0, { 130, 0.14 }, true },
		{ ES47FloraKind::Cholla, 0.0008, 2.2, 240, 1, 0, 0, { 0.86, 1.15 }, 60, 160, 0, 0, 0, { 130, 0.14 }, true },
		// (cap: its own and the dead creosote, which stands in this kind's meshes: Make)
		{ ES47FloraKind::Mesquite, 0.0012, 2.2, 240, 0.8, 0, 0, { 0.62, 1.35 }, 90, 640, 0, 0, 0, { 110, 0.2 }, true },
	};
	const FKind& KindFor(bool bNat, int32 Ki) { return bNat ? NATURAL[Ki] : KINDS[Ki]; }
	// the sway of each kind in Freeform: the grass and the stalks most, the woody shrubs less, stones and stains none
	constexpr double WIND_OF[] = { 1.0, 0.6, 0.4, 1.2, 0.15, 0, 0, 0, 0.5, 1.0, 0.7 };
	// the shrubs that cast a shadow: within this many metres of the eye (the stands are denser now)
	constexpr double SHADOW_CREOSOTE = 24, SHADOW_SAGE = 22, SHADOW_MESQUITE = 30;
	constexpr int32 NK = 11;
	constexpr int32 CREOSOTE_KI = 2, SUCCULENT_KI = 4, STONES_KI = 5, STAIN_KI = 6, ROCK_KI = 7, DESERT_KI = 8, MESQUITE_KI = 10;
	// Freeform's desert plants: a species is one kind (how many, how far: NATURAL), each variant a mesh of /Game/DesertPlants with
	// its share of the species, its sizes, how far it spreads at scale 1 in metres (from its FBX's bounds: none of it
	// may hang over a way or a plot) and the width of its foot (as FFoot's; the stain's rim is about 0.85 at scale 1).
	// Tools/desertplants/import_assets.py imports them. In DESERT_FIRST's order: FKindCell::Part is an index into this.
	struct FDesertPart { const TCHAR* Name; double Share; double Size[2]; double Radius; double Foot; };
	const FDesertPart DESERT_PARTS[] = {
		{ TEXT("SM_DP_Yucca_A"), 0.45, { 0.72, 1.0 }, 0.9, 0.7 },
		{ TEXT("SM_DP_Yucca_B"), 0.30, { 0.95, 1.3 }, 0.85, 0.7 },
		{ TEXT("SM_DP_Yucca_DryStalk"), 0.20, { 0.9, 1.2 }, 0.82, 0.7 },
		{ TEXT("SM_DP_Yucca_Bloom"), 0.05, { 0.78, 1.04 }, 0.85, 0.7 },   // (spring 1986: a few in flower)
		{ TEXT("SM_DP_Cholla_A"), 0.55, { 0.86, 1.0 }, 0.7, 0.5 },
		{ TEXT("SM_DP_Cholla_B"), 0.45, { 1.0, 1.15 }, 0.72, 0.5 },
		{ TEXT("SM_DP_Mesquite_A"), 0.40, { 0.62, 1.0 }, 1.68, 1.2 },
		{ TEXT("SM_DP_Mesquite_B"), 0.60, { 0.76, 1.35 }, 1.3, 1.0 },
	};
	constexpr int32 DESERT_N = UE_ARRAY_COUNT(DESERT_PARTS);
	constexpr int32 DESERT_FIRST[] = { 0, 4, 6, 8 };
	/** Which of a species' variants a number from 0 to 1 picks, by their shares. */
	int32 DesertPartOf(int32 Ki, double Pick)
	{
		const int32 First = DESERT_FIRST[Ki - DESERT_KI], End = DESERT_FIRST[Ki - DESERT_KI + 1];
		double Sum = 0;
		for (int32 Pa = First; Pa < End - 1; ++Pa)
		{
			Sum += DESERT_PARTS[Pa].Share;
			if (Pick < Sum) return Pa;
		}
		return End - 1;
	}
	// Freeform's creosote from the models of /Game/DesertPlants (EnsureCreosote), a mesh to each variant with its share of the kind. Where and
	// how big stay the kind's (NATURAL, Make): the copies' matrices are the same as with the old cards, so the feet, the nursing, the fade and
	// the dead bushes read the same numbers. Heights at scale 1: A 1.6 m, B 1.38 m, C 1.1 m (spec.py); the cards stood 1.6 m.
	struct FCreosotePart { const TCHAR* Name; double Share; };
	const FCreosotePart CREOSOTE_PARTS[] = { { TEXT("SM_DP_Creosote_A"), 0.4 }, { TEXT("SM_DP_Creosote_B"), 0.35 }, { TEXT("SM_DP_Creosote_C"), 0.25 } };
	constexpr int32 CREOSOTE_N = UE_ARRAY_COUNT(CREOSOTE_PARTS);
	int32 CreosotePartOf(double Pick)
	{
		double Sum = 0;
		for (int32 Pa = 0; Pa < CREOSOTE_N - 1; ++Pa)
		{
			Sum += CREOSOTE_PARTS[Pa].Share;
			if (Pick < Sum) return Pa;
		}
		return CREOSOTE_N - 1;
	}
	// Freeform: how much of the stain's colour shows, so that a foot darkens the ground by about a third whatever its colour
	constexpr float FOOT_OPACITY = 0.6f;
	// Freeform: no creosote over 2 m (its shape stands 1.6 m at scale 1); as wide as the references', less of a vase
	constexpr double CREOSOTE_TALLEST = 1.25, CREOSOTE_SPREAD = 1.15;
	// Freeform: mostly small and a few big (the sizes' power), smaller still within 7 m of a way, where the eye comes near its cards
	constexpr double CREOSOTE_SMALLS = 2.4, CREOSOTE_BY_WAY = 0.7, CREOSOTE_WAY_LOOK = 7, WAY_COVER = 0.31;
	// Freeform: this share of the creosote is dead: grey and nearly bare. SM_DP_Mesquite_B (68 % of its cards bare, 16 % in leaf,
	// 1.36 m at scale 1) stands in the kind Mesquite, at these sizes, no new mesh; the creosote's own shape can not lose its leaves.
	constexpr double DEAD_CREOSOTE_SHARE = 0.15, DEAD_SIZE[2] = { 0.5, 0.95 };
	constexpr int32 DEAD_PART = 7;
	// Freeform: of the grass this share is old tufts, grey and dark, 10-40 cm (the card is 0.5 m high at scale 1, Sy 0.8-1.25 of it)
	constexpr double GREY_TUFT_SHARE = 0.2, GREY_TUFT_SIZE[2] = { 0.25, 0.65 };
	// Freeform: what stands round a shrub keeps to it; the grass thickly (the small tufts round the shrubs), the rest as it was
	constexpr double NURSED_GRASS = 1.1, NURSED_REST = 0.5;
	constexpr double CLUMP_K = 4.5;   // the drifts' noise raised to 2.5 averages about 1/4.5
	// NURSE: the grass and the stalks stand round the sage and the creosote
	const TArray<int32>& NurseOf(ES47FloraKind K)
	{
		static const TArray<int32> Shrubs = { 1, 2 }, None;
		return K == ES47FloraKind::Grass || K == ES47FloraKind::Stalks ? Shrubs : None;
	}
	constexpr double C = 8;             // cell size, metres
	constexpr double STEP = 8;          // refill after moving this far; turning keeps the same local plants
	constexpr double SYNC = 30;         // cells this near are made when they are needed; further out ahead of time,
	constexpr double AHEAD_MS = 1.2;    // this long in a picture at the most
	constexpr double JUMP = 40;         // moved further than this between two pictures: everything at once
	constexpr double LOW_REACH = 0.7;   // Low draws this much as far
	constexpr double MAX_R = 260 + STEP;

	/** The cells round the camera's, nearest first: [dx, dz, distance]. */
	struct FOff { int32 Dx, Dz; double Dist; };
	const TArray<FOff>& OFFSETS()
	{
		static TArray<FOff> Out;
		if (Out.Num() == 0)
		{
			const int32 N = int32(FMath::CeilToDouble(MAX_R / C)) + 2;
			for (int32 Dz = -N; Dz <= N; ++Dz) for (int32 Dx = -N; Dx <= N; ++Dx) Out.Add({ Dx, Dz, FMath::Sqrt(double(Dx * Dx + Dz * Dz)) * C });
			Algo::StableSort(Out, [](const FOff& A, const FOff& B) { return A.Dist < B.Dist; });
		}
		return Out;
	}

	// ---------- the colours of the copies (sRGB tints, as the shapes) ----------
	double SrgbToLinear(double V) { return V < 0.04045 ? V * 0.0773993808 : FMath::Pow(V * 0.9478672986 + 0.0521327014, 2.4); }
	FVector Lin(double R, double G, double B) { return FVector(SrgbToLinear(R), SrgbToLinear(G), SrgbToLinear(B)); }
	const TArray<FVector>& PAL(ES47FloraKind K, bool bNat = false)
	{
		static TArray<TArray<FVector>> P, Pn;
		if (bNat)
		{
			// Freeform. These multiply the atlas and the shape's colours (base colour = picture * corner * copy), they
			// are not the plant's colour itself. Creosote: its leaves in the atlas average sRGB (0.23, 0.24, 0.14); these
			// bring them to the references' olive, about sRGB (90, 100, 55). Grass: the atlas's straw is warm and the
			// low sun warmer; these take the orange out, towards the grey beige of the references (154, 140, 120).
			// Stones: the shape's are dark with varnish; the references' are tan and grey with a dark one here and there.
			if (Pn.Num() == 0)
			{
				const TArray<FVector> Sage = PAL(ES47FloraKind::Sage, false), Succ = PAL(ES47FloraKind::Succulent, false), Stain = PAL(ES47FloraKind::Stain, false);
				Pn.SetNum(NK);
				Pn[0] = { Lin(0.93, 1.06, 1.2), Lin(0.88, 1.0, 1.14), Lin(0.98, 1.1, 1.24), Lin(0.84, 0.97, 1.12), Lin(0.95, 1.06, 1.18) };
				Pn[1] = Sage;
				Pn[2] = { Lin(0.86, 1.0, 1.2), Lin(0.8, 0.94, 1.1), Lin(0.92, 1.05, 1.22), Lin(0.84, 1.0, 1.3) };
				Pn[3] = { Lin(1.02, 1.03, 1.05), Lin(0.94, 0.95, 0.97), Lin(1.08, 1.07, 1.06) };
				Pn[4] = Succ;
				Pn[5] = { Lin(1.7, 1.62, 1.55), Lin(1.55, 1.55, 1.6), Lin(1.6, 1.42, 1.3), Lin(1.15, 1.1, 1.05) };
				Pn[6] = Stain;
				Pn[7] = { FVector(1, 1, 1) };   // (the rock's own material takes no colour from the copy)
				// (the atlases and the shapes' corners have the plants' colours already: only a little from one plant to the next)
				// Yucca: its FBX has the green of the Blender pictures, but AgX, a dimmer sun and its own shadow made those dark.
				// By day here all four variants' blades came out at sRGB (193, 170, 100), dead ones (229, 195, 149). These bring the
				// green to about (110, 118, 75) and the dead to grey-tan; the flowers stay cream (they were over 1, clipped).
				Pn[8] = { Lin(0.58, 0.7, 0.77), Lin(0.55, 0.68, 0.74), Lin(0.6, 0.7, 0.75) };
				Pn[9] = { Lin(1.9, 1.6, 1.45), Lin(1.8, 1.55, 1.4), Lin(1.95, 1.6, 1.4) };
				Pn[10] = { Lin(0.78, 0.82, 0.82), Lin(0.74, 0.78, 0.76), Lin(0.8, 0.82, 0.78) };
			}
			return Pn[int32(K)];
		}
		if (P.Num() == 0)
		{
			P.SetNum(NK);
			P[0] = { Lin(1, 0.94, 0.8), Lin(0.94, 0.9, 0.8), Lin(1, 0.85, 0.72), Lin(1.02, 0.98, 0.9), Lin(0.8, 0.74, 0.66) };
			P[1] = { Lin(1, 0.98, 0.9), Lin(0.94, 0.93, 0.84), Lin(1, 0.95, 0.88), Lin(0.9, 0.84, 0.76) };
			P[2] = { Lin(0.98, 1, 0.9), Lin(0.9, 0.92, 0.82), Lin(1.04, 1, 0.86) };
			P[3] = { Lin(1, 1, 1), Lin(0.9, 0.88, 0.85), Lin(1.04, 0.97, 0.9) };
			P[4] = { Lin(0.98, 1, 0.94), Lin(0.92, 0.94, 0.86), Lin(1, 0.97, 0.9) };
			P[5] = { Lin(1, 0.98, 0.95), Lin(0.9, 0.86, 0.82), Lin(1.04, 0.94, 0.86), Lin(0.86, 0.84, 0.82) };
			P[6] = { Lin(0.34, 0.28, 0.22), Lin(0.7, 0.66, 0.58), Lin(0.6, 0.45, 0.34), Lin(0.5, 0.48, 0.44), Lin(0.28, 0.24, 0.19) };
		}
		return P[int32(K)];
	}
	/** Freeform: the creosote's models, in place of Pn[2], which stays for the old cards when the models are not there. Those multiply a dark atlas
	 *  up to olive; this atlas has the olive already and the game's sun lights it strongly, so these start where the mesquite's do (Pn[10]). */
	const TArray<FVector>& CREOSOTE_MODEL_PAL()
	{
		// Lime yellow at 11:31: the lit leaves read 170,143,74. Each row scaled alike toward olive grey, 95,105,62 in the sun (linear .28 .51 .68). The variation stays.
		static const TArray<FVector> V = { Lin(0.448, 0.602, 0.653), Lin(0.426, 0.587, 0.619), Lin(0.47, 0.617, 0.636), Lin(0.437, 0.602, 0.686) };
		return V;
	}
	/** Broom snakeweed, the commonest little shrub of the grassland: a sage copy, smaller, yellow-green. */
	const FVector& SNAKEWEED() { static const FVector V = Lin(0.98, 1.04, 0.66); return V; }
	constexpr double SNAKEWEED_SHARE = 0.32;
	/** The litter under a big tuft or a shrub (the stains): old leaves and shade, dark. */
	const FVector& LITTER(int32 I) { static const FVector V[2] = { Lin(0.22, 0.18, 0.14), Lin(0.27, 0.22, 0.17) }; return V[I]; }
	constexpr double LITTER_SHARE = 0.25;
	/** Freeform: the soft dark foot under a shrub (shown at FOOT_OPACITY): dark brown, the ground through it. */
	const FVector& FOOT(int32 I) { static const FVector V[2] = { Lin(0.38, 0.32, 0.26), Lin(0.33, 0.28, 0.23) }; return V[I]; }
	/** A pale caliche stone now and then among the dark ones. */
	const FVector& CALICHE() { static const FVector V = Lin(1.55, 1.48, 1.32); return V; }
	/** Freeform: a dead creosote's bare twigs (they multiply Mesquite_B's atlas, whose thin twigs show dark here), grey as reference 10's. */
	const FVector& DEAD_TWIGS(int32 I) { static const FVector V[2] = { Lin(0.8, 0.77, 0.75), Lin(0.7, 0.68, 0.67) }; return V[I]; }
	/** Freeform: the dead creosote when the desert meshes are not there: its own shape, grey and dark. */
	const FVector& DEAD_CREOSOTE() { static const FVector V = Lin(0.55, 0.55, 0.56); return V; }
	/** Freeform: an old tuft of grass, multiplying the kind's colour: grey-brown and darker. */
	const FVector& GREY_TUFT(int32 I) { static const FVector V[2] = { Lin(0.68, 0.65, 0.62), Lin(0.6, 0.58, 0.56) }; return V[I]; }

	bool Inside(const FS47FloraRect& B, double X, double Z, double M = 0) { return X >= B.MinX - M && X <= B.MaxX + M && Z >= B.MinZ - M && Z <= B.MaxZ + M; }
	double Fmod1(double V) { return V - FMath::FloorToDouble(V); } // (a % 1 of a non-negative number)

	// ---------- three's Quaternion, in its own terms (x, y, z, w) ----------
	FQuat QMul(const FQuat& A, const FQuat& B)
	{
		// multiplyQuaternions(a, b)
		return FQuat(A.X * B.W + A.W * B.X + A.Y * B.Z - A.Z * B.Y, A.Y * B.W + A.W * B.Y + A.Z * B.X - A.X * B.Z,
			A.Z * B.W + A.W * B.Z + A.X * B.Y - A.Y * B.X, A.W * B.W - A.X * B.X - A.Y * B.Y - A.Z * B.Z);
	}
	FQuat QAxisAngle(const FVector& Axis, double Angle)
	{
		const double H = Angle / 2, S = FMath::Sin(H);
		return FQuat(Axis.X * S, Axis.Y * S, Axis.Z * S, FMath::Cos(H));
	}
	FQuat QUnitVectors(const FVector& From, const FVector& To)
	{
		double R = FVector::DotProduct(From, To) + 1;
		FQuat Q;
		if (R < 2.220446049250313e-16)
		{
			R = 0;
			Q = FMath::Abs(From.X) > FMath::Abs(From.Z) ? FQuat(-From.Y, From.X, 0, R) : FQuat(0, -From.Z, From.Y, R);
		}
		else Q = FQuat(From.Y * To.Z - From.Z * To.Y, From.Z * To.X - From.X * To.Z, From.X * To.Y - From.Y * To.X, R);
		const double L = FMath::Sqrt(Q.X * Q.X + Q.Y * Q.Y + Q.Z * Q.Z + Q.W * Q.W);
		return L == 0 ? FQuat(0, 0, 0, 1) : FQuat(Q.X / L, Q.Y / L, Q.Z / L, Q.W / L);
	}
	FVector Normalize(const FVector& V) { const double L = V.Size(); return L > 0 ? V / L : V; }
	const FVector UpV(0, 1, 0);
}

// ---------- random numbers ----------

namespace S47Flora
{
uint32 Hash4(int32 A, int32 B, int32 C, int32 D)
{
	// flora.ts hash4, with Math.imul's 32-bit products and >>>'s unsigned shifts
	uint32 H = 2166136261u ^ (uint32(A) * 0x27d4eb2du);
	H = (H ^ uint32(B)) * 16777619u; H = (H ^ uint32(C)) * 0x165667b1u; H = (H ^ uint32(D)) * 0x9e3779b1u;
	H ^= H >> 15; H = H * 0x85ebca6bu; H ^= H >> 13;
	return H;
}

double FMulberry::operator()()
{
	S = S + 0x6d2b79f5u;
	uint32 T = (S ^ (S >> 15)) * (1u | S);
	T = (T + (T ^ (T >> 7)) * (61u | T)) ^ T;
	return double(T ^ (T >> 14)) / 4294967296.0;
}

double VNoise(double X, double Z, double Size, int32 Seed)
{
	// Smooth value noise, 0..1, on a lattice of `size` metres
	const double U = X / Size, V = Z / Size, I = FMath::FloorToDouble(U), J = FMath::FloorToDouble(V), Fu = U - I, Fv = V - J;
	auto H = [Seed](double A, double B) { return Hash4(int32(A), int32(B), Seed, 77) / 4294967296.0; };
	const double Su = Fu * Fu * (3 - 2 * Fu), Sv = Fv * Fv * (3 - 2 * Fv);
	const double Ha = H(I, J), Hb = H(I + 1, J), Hc = H(I, J + 1), Hd = H(I + 1, J + 1);
	return Ha + (Hb - Ha) * Su + (Hc - Ha) * Sv + (Ha - Hb - Hc + Hd) * Su * Sv;
}

double Ragged(double X, double Z) { return VNoise(X, Z, 2.3, 4111); }
}

using namespace S47Flora;

// ---------- Freeform's wide fields ----------
// Two sizes of noise through a narrow threshold: a clear edge between where a kind stands and where it
// nearly does not. Tried outside the engine on 480 m of open desert: the stands cover about 31 % of it, the
// bushes in them are 0.5-3.3 m from the nearest (CreosoteSpot); the meadows about 58 %.
namespace
{
	/** Where the creosote stands, 0 bare, 1 a stand: patches a hundred metres across, torn at the edges. */
	double Stand(double X, double Z, int32 Seed)
	{
		return FMath::SmoothStep(0.51, 0.62, 0.68 * VNoise(X, Z, 90, Seed + 31) + 0.32 * VNoise(X, Z, 26, Seed + 37));
	}
	/** Where the grass and the dead weeds grow, 0 bare, 1 a meadow. */
	double Meadow(double X, double Z, int32 Seed)
	{
		return FMath::SmoothStep(0.44, 0.56, 0.62 * VNoise(X, Z, 64, Seed + 917) + 0.38 * VNoise(X, Z, 19, Seed + 919));
	}
	/** Where the stones lie, 0 few, 1 a cluster: a wash of gravel some tens of metres across, in knots. */
	double StoneField(double X, double Z, int32 Seed)
	{
		return FMath::SmoothStep(0.5, 0.62, 0.6 * VNoise(X, Z, 34, Seed + 41) + 0.4 * VNoise(X, Z, 7, Seed + 43));
	}
	/** Where candidate T of a cell's nine creosote stands: its 2.7 m square, jittered past the square's edges, then the whole
	 *  pulled about by a noise of 17 m, so no line of them runs through the cells. Outside the engine: the candidates' pull to
	 *  the old 2.67 m lines fell from 0.24 to 0.03, Clark-Evans from 1.30 to 1.13 (1 is random), the nearest bush at the median 2.07 to 1.80 m. */
	void CreosoteSpot(int32 I, int32 J, int32 T, double U, double V, int32 Seed, double& X, double& Z)
	{
		const double X0 = (I + ((T % 3) + 0.5 + 1.25 * (U - 0.5)) / 3) * C, Z0 = (J + ((T / 3) + 0.5 + 1.25 * (V - 0.5)) / 3) * C;
		X = X0 + 5 * (VNoise(X0, Z0, 17, Seed + 61) - 0.5);
		Z = Z0 + 5 * (VNoise(X0, Z0, 17, Seed + 67) - 0.5);
	}
}

// ---------- the patch ----------

FS47Flora::FS47Flora(const FS47FloraSpec& InSpec) : Spec(InSpec)
{
	Add(Spec.Blocks, Spec.Walks);
}

void FS47Flora::Add(const TArray<FS47FloraRect>& Blocks, const TArray<FS47FloraRect>& Walks)
{
	auto PutR = [this](const FS47FloraRect& R, bool bW)
	{
		const double M = 1.4;
		for (int32 I = int32(FMath::FloorToDouble((R.MinX - M) / 16)); I <= int32(FMath::FloorToDouble((R.MaxX + M) / 16)); ++I)
			for (int32 J = int32(FMath::FloorToDouble((R.MinZ - M) / 16)); J <= int32(FMath::FloorToDouble((R.MaxZ + M) / 16)); ++J)
			{
				FBucket& E = Bucket.FindOrAdd(int64(I) * 65536 + J);
				(bW ? E.W : E.B).Add(R);
			}
		// the cells it touches are made again
		for (int32 I = int32(FMath::FloorToDouble((R.MinX - M) / C)); I <= int32(FMath::FloorToDouble((R.MaxX + M) / C)); ++I)
			for (int32 J = int32(FMath::FloorToDouble((R.MinZ - M) / C)); J <= int32(FMath::FloorToDouble((R.MaxZ + M) / C)); ++J) Cells.Remove(Key(I, J));
	};
	for (const FS47FloraRect& R : Blocks) PutR(R, false);
	for (const FS47FloraRect& R : Walks) PutR(R, true);
	bDirty = true;
}

double FS47Flora::NoiseAt(double X, double Z, int32 Ki, double Terrain) const
{
	const FKind& K = KindFor(Spec.bNatural, Ki);
	if (Spec.bNatural && K.Id == ES47FloraKind::Creosote)
	{
		// dense stands, thinner here and there inside; between them a bush some tens of metres apart
		const double St = Stand(X, Z, Spec.Seed);
		const double Field = St > 0 ? St * (0.5 + 0.5 * VNoise(X, Z, 11, Spec.Seed + 39)) : 0;
		const double* Mx = Spec.Mix.Find(K.Id);
		return (0.012 + 0.95 * Field) * (Mx ? *Mx : 1);
	}
	if (Spec.bNatural && K.Id == ES47FloraKind::Grass)
	{
		// tufts in drifts and in bands (stretched 3 : 1 along a line of runoff, 35 degrees), dirt between;
		// thicker in the lows of the ground (Terrain, from Make), none in the lightest third of the noise;
		// only in its meadows, and thin among the creosote (the farm's pasture a meadow all over)
		const double Iso = VNoise(X, Z, K.Clump, Spec.Seed + 911);
		const double U = X * 0.8192 + Z * 0.5736, V = -X * 0.5736 + Z * 0.8192;
		const double Band = VNoise(U / 3.0, V, 5.0, Spec.Seed + 913);
		const double Live = FMath::Max(0.0, (0.5 * Iso + 0.5 * Band - 0.3) / 0.7);
		if (Live <= 0) return 0;
		double Mead = Meadow(X, Z, Spec.Seed);
		if (Spec.Farm && Mead < 1) { const double F = Spec.Farm(X, Z, K.Id); if (F > 1) Mead = FMath::Max(Mead, 0.8 * FMath::Min(1.0, (F - 1) / 1.3)); }
		const double* Mx = Spec.Mix.Find(K.Id);
		return 6.0 * Live * Live * Terrain * (0.05 + 0.95 * Mead) * (1 - 0.35 * Stand(X, Z, Spec.Seed)) * (Mx ? *Mx : 1);
	}
	if (Spec.bNatural && K.Id == ES47FloraKind::Rock)
	{
		const double* Mx = Spec.Mix.Find(K.Id);
		return (0.04 + 1.1 * StoneField(X, Z, Spec.Seed)) * (Mx ? *Mx : 1);
	}
	// (each desert species its own patches, not those of the kinds with its size of patch)
	const int32 Salt = Ki >= DESERT_KI ? 100 * (Ki - DESERT_KI + 1) : 0;
	double N = K.Patch > 1 ? 0.3 + 1.4 * VNoise(X, Z, K.Patch, Spec.Seed + int32(K.Patch) + Salt) : 1;
	// drifts a few metres across with bare dirt between (raised to a power: most of it thin, some thick)
	if (K.Clump > 0) N *= CLUMP_K * FMath::Pow(VNoise(X, Z, K.Clump, Spec.Seed + 911), 2.5);
	// Freeform: the sage in its own wide patches, the dead weeds where the grass is
	if (Spec.bNatural && K.Id == ES47FloraKind::Sage) N *= 0.06 + 1.6 * FMath::SmoothStep(0.46, 0.58, VNoise(X, Z, 47, Spec.Seed + 51));
	else if (Spec.bNatural && K.Id == ES47FloraKind::Stalks) N *= 0.1 + 0.9 * Meadow(X, Z, Spec.Seed);
	const double* Mx = Spec.Mix.Find(K.Id);
	return N * (Mx ? *Mx : 1);
}

double FS47Flora::CoverAt(double X, double Z, int32 Ki, double Noise)
{
	const FKind& K = KindFor(Spec.bNatural, Ki);
	bWalked = false; bSeamed = false; bSpilt = false;
	double D = Spec.Cover ? Spec.Cover(X, Z) : 1;
	if (D <= 0)
	{
		// on a way: only in its seams, tiny grass and pebbles
		if (K.Seam <= 0 || !Spec.Seam) return 0;
		D = Spec.Seam(X, Z) * K.Seam;
		if (D <= 0) return 0;
		bSeamed = true;
	}
	if (const FBucket* E = Bucket.Find(int64(FMath::FloorToDouble(X / 16)) * 65536 + int64(FMath::FloorToDouble(Z / 16))))
	{
		bool bNear = false;
		for (const FS47FloraRect& B : E->B) { if (Inside(B, X, Z, 0.12)) return 0; if (!bNear && Inside(B, X, Z, 1.3)) bNear = true; }
		if (bNear && !bSeamed) D *= 1.7;
		for (const FS47FloraRect& W : E->W) if (Inside(W, X, Z)) { bWalked = true; break; }
	}
	if (K.Id == ES47FloraKind::Stones && !bSeamed && Spec.Spill) { const double S = Spec.Spill(X, Z); if (S > 1) { D *= S; bSpilt = true; } }
	if (Spec.bNatural)
	{
		// the new buildings' plots (nothing grows), the trampled ground, and the farm's yard and pasture
		for (const FS47FloraRect& B : Spec.NatBlocks) if (Inside(B, X, Z)) return 0;
		for (const FS47FloraRect& W : Spec.NatWalks) if (Inside(W, X, Z)) { bWalked = true; break; }
		if (Spec.Farm) { const double F = Spec.Farm(X, Z, K.Id); if (F <= 0) return 0; D *= F; }
	}
	return D * Noise;
}

const FS47Flora::FKindCell& FS47Flora::Cell(int32 I, int32 J, int32 Ki)
{
	const int64 K = Key(I, J);
	TArray<TOptional<FKindCell>>* Cs = Cells.Find(K);
	if (!Cs) { Cs = &Cells.Add(K); Cs->SetNum(NK); }
	if (!(*Cs)[Ki].IsSet())
	{
		FKindCell Made = Make(I, J, Ki);
		// (Make may have made the cell's other kinds, under the same key: the entry is where it was)
		Cs = Cells.Find(K);
		(*Cs)[Ki] = MoveTemp(Made);
	}
	return *(*Cs)[Ki];
}

TArray<float> FS47Flora::Nurses(int32 I, int32 J, const TArray<int32>& Kinds)
{
	// the shrubs of a cell the grass and the stalks stand round, as [x, z, size, ...]
	TArray<float> Out;
	for (int32 Ki : Kinds)
	{
		const FKindCell& Cc = Cell(I, J, Ki);
		for (int32 T = 0; T < Cc.N; ++T)
		{
			const float* M = Cc.M.GetData() + T * 16;
			Out.Add(M[12]); Out.Add(M[14]); Out.Add(float(FMath::Sqrt(double(M[0]) * M[0] + double(M[1]) * M[1] + double(M[2]) * M[2])));
		}
	}
	return Out;
}

FS47Flora::FKindCell FS47Flora::Make(int32 I, int32 J, int32 Ki)
{
	// A cell's copies of a kind: drawn the same every time from the cell's own random numbers (the
	// same count of them for every candidate, kept or not), sorted by rank so Low keeps the first.
	const bool bNat = Spec.bNatural;
	const FKind& K = KindFor(bNat, Ki);
	FMulberry R(Hash4(I, J, Ki, Spec.Seed));
	const double Want = K.Rate * C * C * K.Most;
	const bool bStones = K.Id == ES47FloraKind::Stones, bStain = K.Id == ES47FloraKind::Stain, bRock = K.Id == ES47FloraKind::Rock;
	// Freeform's creosote: nine candidates in a cell (CreosoteSpot), then thinned by the stands; no stain of its own
	// (only the feet, below), no rock when its mesh is not there; the rest as before
	const bool bGrid = bNat && K.Id == ES47FloraKind::Creosote;
	// Freeform's desert plants: only when all their meshes are there, and then instead of the old succulent copies (never both)
	const bool bDesert = Ki >= DESERT_KI;
	const bool bNone = bNat && (bStain || (bRock && RockUnit <= 0) || (bDesert && !bDesertOn) || (K.Id == ES47FloraKind::Succulent && bDesertOn));
	const int32 N = bGrid ? 9 : bNone ? 0 : int32(FMath::FloorToDouble(Want)) + (R() < FMath::Fmod(Want, 1.0) ? 1 : 0);
	struct FOut { float M[16]; float Cl[3]; double Rank; bool bSeam; uint8 Part; };
	uint8 PartNow = 0;
	TArray<FOut> Out;
	// Freeform's creosote from its models: which of them, from N2 (no other use for it in this kind), and their own colours
	const bool bCreosoteModel = bGrid && bCreosoteOn;
	const TArray<FVector>& Pal = bCreosoteModel ? CREOSOTE_MODEL_PAL() : PAL(K.Id, bNat);
	const TFunction<double(double, double)>& G = Spec.Ground;
	auto Gy = [&G](double X, double Z) { return G ? G(X, Z) : 0.0; };
	const TArray<int32>& NurseKinds = NurseOf(K.Id);
	const TArray<float> Nurse = K.Nurse > 0 && NurseKinds.Num() ? Nurses(I, J, NurseKinds) : TArray<float>();
	const int32 Nn = Nurse.Num() / 3;
	FQuat Qt(0, 0, 0, 1);
	FVector Sc(1, 1, 1);
	auto PutOne = [&](double X, double Y, double Z, const FVector& Cl, double Rank, bool bSeam)
	{
		double E[16];
		S47Three::Compose(FVector(X, Y, Z), Qt, Sc, E);
		FOut O;
		for (int32 k = 0; k < 16; ++k) O.M[k] = float(E[k]);
		O.Cl[0] = float(Cl.X); O.Cl[1] = float(Cl.Y); O.Cl[2] = float(Cl.Z);
		O.Rank = Rank; O.bSeam = bSeam; O.Part = PartNow;
		Out.Add(O);
	};
	auto Tint = [&Pal](double CA, double CB, double F)
	{
		const int32 L = Pal.Num();
		const FVector& C0 = Pal[int32(FMath::FloorToDouble(CA * L)) % L];
		const FVector& C1 = Pal[int32(FMath::FloorToDouble(CB * L)) % L];
		return C0 + (C1 - C0) * F;
	};
	// Freeform's grass grows thicker in the lows of the ground: how far the ground lies under the mean of
	// its neighbours 8 m off, at the cell's four corners (twenty samples a cell, not a hundred)
	double Low4[4] = { 0, 0, 0, 0 };
	if (bNat && K.Id == ES47FloraKind::Grass && G)
	{
		auto Lowness = [&G](double Px, double Pz) { return 0.25 * (G(Px + 8, Pz) + G(Px - 8, Pz) + G(Px, Pz + 8) + G(Px, Pz - 8)) - G(Px, Pz); };
		Low4[0] = Lowness(I * C, J * C); Low4[1] = Lowness((I + 1) * C, J * C); Low4[2] = Lowness(I * C, (J + 1) * C); Low4[3] = Lowness((I + 1) * C, (J + 1) * C);
	}
	for (int32 T = 0; T < N; ++T)
	{
		const double U = R(), V = R(), A = R(), B = R(), CA = R(), CB = R(), Ry = R(), L1 = R(), L2 = R(), Rank = R(), N1 = R(), N2 = R(), N3 = R();
		double X = (I + U) * C, Z = (J + V) * C;
		if (bGrid) CreosoteSpot(I, J, T, U, V, Spec.Seed, X, Z);
		// most of the grass and the stalks stand 0.4 to 1.5 m from a shrub of the cell
		const bool bNursed = Nn && N1 < K.Nurse;
		if (bNursed)
		{
			const int32 Qi = int32(FMath::FloorToDouble(N2 * Nn)) * 3;
			const double An = N3 * UE_DOUBLE_PI * 2, Dist = Nurse[Qi + 2] * 0.3 + 0.4 + V * 1.1;
			X = Nurse[Qi] + FMath::Cos(An) * Dist; Z = Nurse[Qi + 1] + FMath::Sin(An) * Dist;
		}
		double Terrain = 1;
		if (bNat && K.Id == ES47FloraKind::Grass)
		{
			const double Fx = FMath::Clamp(X / C - I, 0.0, 1.0), Fz = FMath::Clamp(Z / C - J, 0.0, 1.0);
			const double Lo = (Low4[0] * (1 - Fx) + Low4[1] * Fx) * (1 - Fz) + (Low4[2] * (1 - Fx) + Low4[3] * Fx) * Fz;
			Terrain = FMath::Clamp(1 + Lo * 8, 0.4, 2.2);
		}
		double Noise = NoiseAt(X, Z, Ki, Terrain);
		// Freeform: what stands round a shrub keeps to it, also in the bare fields between the meadows
		if (bNat && bNursed) Noise = FMath::Max(Noise, K.Id == ES47FloraKind::Grass ? NURSED_GRASS : NURSED_REST);
		if (A * K.Most >= Noise * 4.2) continue;   // (the cover is at most about that much, near a wall in a ditch)
		double D = CoverAt(X, Z, Ki, Noise);
		// Freeform's stones: gravel thickens with the slope (over 6 m) and lies in clusters with little between
		double SlopeF = 1;
		if (bNat && bStones && !bSeamed && !bSpilt)
		{
			const double Sx = Gy(X + 3, Z) - Gy(X - 3, Z), Sz = Gy(X, Z + 3) - Gy(X, Z - 3);
			SlopeF = FMath::Clamp(FMath::Sqrt(Sx * Sx + Sz * Sz) / 6 / 0.18, 0.0, 1.0);
			D *= (0.3 + 3.0 * SlopeF) * (0.1 + 2.4 * StoneField(X, Z, Spec.Seed));
		}
		if (A * K.Most >= FMath::Min(D, K.Most)) continue;
		// Freeform: a dead creosote stands in Mesquite's cell instead (Make, below), from these same numbers
		const bool bDeadBush = bGrid && N1 < DEAD_CREOSOTE_SHARE;
		if (bDeadBush && bDesertOn) continue;
		// big where it is thick, small and mostly tiny in the open
		const double Dens = FMath::Min(1.0, D / K.Most);
		double S = K.Size[0] + FMath::Pow(B, 2.5 - 1.7 * Dens + (bStones ? 1.3 : 0)) * (K.Size[1] - K.Size[0]);
		// Freeform's creosote: whatever the density (the stands' density made them all big), mostly small, smaller by a way
		if (bNat && K.Id == ES47FloraKind::Creosote)
		{
			S = K.Size[0] + FMath::Pow(B, CREOSOTE_SMALLS) * (K.Size[1] - K.Size[0]);
			if (Spec.Cover && S > CREOSOTE_BY_WAY && (Spec.Cover(X + CREOSOTE_WAY_LOOK, Z) <= WAY_COVER || Spec.Cover(X - CREOSOTE_WAY_LOOK, Z) <= WAY_COVER
				|| Spec.Cover(X, Z + CREOSOTE_WAY_LOOK) <= WAY_COVER || Spec.Cover(X, Z - CREOSOTE_WAY_LOOK) <= WAY_COVER)) S = CREOSOTE_BY_WAY;
		}
		// Freeform's old tufts among the grass
		const bool bGreyTuft = bNat && K.Id == ES47FloraKind::Grass && Fmod1(Ry * 17.3) < GREY_TUFT_SHARE;
		if (bGreyTuft) S = GREY_TUFT_SIZE[0] + B * (GREY_TUFT_SIZE[1] - GREY_TUFT_SIZE[0]);
		// Freeform's desert plants: which of the species (N2 has no other use for them), in that variant's sizes, whatever the density
		if (bDesert)
		{
			PartNow = uint8(DesertPartOf(Ki, N2));
			const FDesertPart& Dp = DESERT_PARTS[PartNow];
			S = Dp.Size[0] + FMath::Pow(B, 1.2) * (Dp.Size[1] - Dp.Size[0]);
		}
		if (bCreosoteModel) PartNow = uint8(CreosotePartOf(N2));
		if (bWalked) { if (K.Walk <= 0 || Fmod1(CB * 7.31) > K.Walk) continue; S = FMath::Min(S, K.WalkMax); }
		// in a seam tiny grass and pebbles; the gravel spilt off a way small
		if (bSeamed) S = K.Id == ES47FloraKind::Grass ? 0.25 + B * 0.2 : 0.3 + B * 0.2;
		else if (bSpilt) S = FMath::Min(S, 0.3 + B * 0.35);
		// Freeform's stones: 2-15 cm in the middle distance (the shape's rock is 0.44 across at scale 1, its pebbles
		// less; the big ones are Rock's), bigger with the slope
		else if (bNat && bStones)
		{
			S = 0.12 + (0.33 + 1.7 * SlopeF) * FMath::Pow(B, 1.5);
			if (bWalked) S = FMath::Min(S, K.WalkMax);
		}
		// the whole of a desert plant off every way, plot, pad and walk, not only its root
		if (bDesert)
		{
			const double Spread = DESERT_PARTS[PartNow].Radius * S;
			bool bClear = true;
			for (int32 Round = 0; Round < 8 && bClear; ++Round)
			{
				const double Turn = Round * UE_DOUBLE_PI / 4;
				bClear = CoverAt(X + FMath::Cos(Turn) * Spread, Z + FMath::Sin(Turn) * Spread, Ki, 1.0) > 0 && !bWalked && !bSeamed;
			}
			if (!bClear) continue;
		}
		const double Y = Gy(X, Z);
		double Sy = S;
		FVector Col = Tint(CA, CB, L2 * 0.5);
		// Freeform: the dry and the green shift a little from one area to the next
		if (bNat && (K.Id == ES47FloraKind::Grass || K.Id == ES47FloraKind::Stalks || K.Id == ES47FloraKind::Creosote))
		{
			const double Dry = VNoise(X, Z, 160, Spec.Seed + 57);
			Col = Col * FVector(0.93 + 0.14 * Dry, 1.0, 1.07 - 0.14 * Dry);
		}
		if (bDeadBush) Col = DEAD_CREOSOTE();
		if (bGreyTuft) Col = Col * GREY_TUFT(L2 < 0.5 ? 0 : 1);
		if (bRock)
		{
			// Freeform's rock: Sandstone_A S metres across, flat on the ground's slope, its foot in the dirt
			const double H = 0.4, Su = S * RockUnit;
			const FVector Nrm = Normalize(FVector(-(Gy(X + H, Z) - Gy(X - H, Z)) / (2 * H), 1, -(Gy(X, Z + H) - Gy(X, Z - H)) / (2 * H)));
			Qt = QMul(QUnitVectors(UpV, Nrm), QAxisAngle(UpV, Ry * UE_DOUBLE_PI * 2));
			Sc = FVector(Su * (0.85 + 0.3 * L2), Su * (0.7 + 0.4 * L1), Su);
			PutOne(X, Y + RockFoot * Sc.Y, Z, Col, Rank, false);
		}
		else if (bStones || bStain)
		{
			// flat on the ground's slope; a stain only where the ground is near flat across it
			const double H = bStain ? S : 0.4;
			const double Hx = Gy(X + H, Z) - Gy(X - H, Z), Hz = Gy(X, Z + H) - Gy(X, Z - H);
			if (bStain && FMath::Abs(Gy(X + H, Z) + Gy(X - H, Z) - 2 * Y) + FMath::Abs(Gy(X, Z + H) + Gy(X, Z - H) - 2 * Y) > 0.05) continue;
			const FVector Nrm = Normalize(FVector(-Hx / (2 * H), 1, -Hz / (2 * H)));
			Qt = QMul(QUnitVectors(UpV, Nrm), QAxisAngle(UpV, Ry * UE_DOUBLE_PI * 2));
			if (bStones)
			{
				Qt = QMul(Qt, S47Three::Euler((L1 - 0.5) * 0.4, 0, (L2 - 0.5) * 0.4));
				Sy = S * (0.7 + L1 * 0.6);
				if (N1 < (bNat ? 0.18 : 0.06)) Col = bNat ? CALICHE() : Col * CALICHE();
			}
			Sc = FVector(S * (bStain ? 0.8 + L2 * 0.5 : 1), Sy, S);
			// the stones a third in the dirt; the stains (ranked first: litter comes after them) on it
			PutOne(X, Y + (bStain ? 0.02 : -0.025 * Sy), Z, Col, bStain ? Rank * 0.5 : Rank, bSeamed);
		}
		else
		{
			const double Lean = K.Id == ES47FloraKind::Grass ? 0.16 : K.Id == ES47FloraKind::Creosote ? 0.1 : 0.08;
			Qt = S47Three::Euler((L1 - 0.5) * Lean * 2, Ry * UE_DOUBLE_PI * 2, (L2 - 0.5) * Lean * 2, TEXT("YXZ"));
			Sy = S * (0.8 + L1 * 0.45);
			// a third of the sage copies are snakeweed: smaller, rounder, yellow-green
			if (K.Id == ES47FloraKind::Sage && N2 < SNAKEWEED_SHARE && !Nn) { S *= 0.62; Sy = S * (0.75 + L1 * 0.3); Col = Col * SNAKEWEED(); }
			// two in five succulents are cholla: their copies are mirrored (succulentShape)
			Sc = FVector(K.Id == ES47FloraKind::Succulent && Fmod1(CA * 13.7) < 0.4 ? -S : S, Sy, S);
			if (bNat && K.Id == ES47FloraKind::Creosote) Sc = FVector(S * CREOSOTE_SPREAD, FMath::Min(Sy, CREOSOTE_TALLEST), S * CREOSOTE_SPREAD);
			// (the models have their own shapes: little more height from one to the next, and never mirrored)
			if (bDesert) { Sy = S * (0.94 + L1 * 0.12); Sc = FVector(S, Sy, S); }
			PutOne(X, Y - 0.035 * S, Z, Col, Rank, bSeamed);
		}
	}
	// Freeform: a soft dark foot under every shrub and two in five of the big tufts, about as wide as the plant
	// (0.6-1.2 of its radius; the stain's rim is about 0.85 at scale 1, the plant's top 0.54-0.68 of its size).
	// A shadow proxy cannot do this: its shadow goes with the sun and is gone at noon and in the shade.
	if (bStain && bNat)
	{
		auto PutFoot = [&](const float* Cm, double Big, double Width, double Share)
		{
			const double S = FMath::Sqrt(double(Cm[0]) * Cm[0] + double(Cm[1]) * Cm[1] + double(Cm[2]) * Cm[2]), H1 = R(), H2 = R(), H3 = R();
			if (S < Big || H1 >= Share) return;
			const double X = Cm[12] + (H2 - 0.5) * 0.15 * S, Z = Cm[14] + (H3 - 0.5) * 0.15 * S, Y = Gy(X, Z), Sz = S * Width * (0.7 + 0.6 * H2);
			if (FMath::Abs(Gy(X + Sz, Z) + Gy(X - Sz, Z) - 2 * Y) + FMath::Abs(Gy(X, Z + Sz) + Gy(X, Z - Sz) - 2 * Y) > 0.05) return;
			const FVector Nrm = Normalize(FVector(-(Gy(X + Sz, Z) - Gy(X - Sz, Z)) / (2 * Sz), 1, -(Gy(X, Z + Sz) - Gy(X, Z - Sz)) / (2 * Sz)));
			Qt = QMul(QUnitVectors(UpV, Nrm), QAxisAngle(UpV, H3 * UE_DOUBLE_PI * 2));
			Sc = FVector(Sz * (0.85 + 0.3 * H1), Sz, Sz);
			PutOne(X, Y + 0.018, Z, FOOT(H2 < 0.5 ? 0 : 1), H3, false);
		};
		struct FFoot { int32 Kind; double Big, Width, Share; };
		for (const FFoot& L : { FFoot{ 0, 0.55, 0.5, 0.4 }, FFoot{ 1, 0.3, 0.57, 1.0 }, FFoot{ 2, 0.0, 0.62, 1.0 } })
		{
			const FKindCell Cc = Cell(I, J, L.Kind);
			for (int32 T = 0; T < Cc.N; ++T) PutFoot(Cc.M.GetData() + T * 16, L.Big, L.Width, L.Share);
		}
		// (no copy of a desert plant when their meshes are not there, so nothing changes then)
		for (int32 Dk = DESERT_KI; Dk < NK; ++Dk)
		{
			const FKindCell Cc = Cell(I, J, Dk);
			for (int32 T = 0; T < Cc.N; ++T) PutFoot(Cc.M.GetData() + T * 16, 0.0, DESERT_PARTS[Cc.Part[T]].Foot, 1.0);
		}
	}
	// the litter under one in four of the big tufts and the shrubs: a small dark stain, ranked after
	// the stains (Kind.far: near the camera only)
	else if (bStain)
	{
		struct FLit { int32 Kind; double Big, W; };
		for (const FLit& L : { FLit{ 0, 1.0, 0.42 }, FLit{ 1, 0.8, 0.52 }, FLit{ 2, 0.7, 0.5 } })
		{
			const FKindCell Cc = Cell(I, J, L.Kind);
			for (int32 T = 0; T < Cc.N; ++T)
			{
				const float* M = Cc.M.GetData() + T * 16;
				const double S = FMath::Sqrt(double(M[0]) * M[0] + double(M[1]) * M[1] + double(M[2]) * M[2]), H1 = R(), H2 = R(), H3 = R();
				if (S < L.Big || H1 >= LITTER_SHARE) continue;
				const double X = M[12] + (H2 - 0.5) * 0.2 * S, Z = M[14] + (H3 - 0.5) * 0.2 * S, Y = Gy(X, Z), Sz = S * L.W * (0.85 + H2 * 0.3);
				if (FMath::Abs(Gy(X + Sz, Z) + Gy(X - Sz, Z) - 2 * Y) + FMath::Abs(Gy(X, Z + Sz) + Gy(X, Z - Sz) - 2 * Y) > 0.05) continue;
				const FVector Nrm = Normalize(FVector(-(Gy(X + Sz, Z) - Gy(X - Sz, Z)) / (2 * Sz), 1, -(Gy(X, Z + Sz) - Gy(X, Z - Sz)) / (2 * Sz)));
				Qt = QMul(QUnitVectors(UpV, Nrm), QAxisAngle(UpV, H3 * UE_DOUBLE_PI * 2));
				Sc = FVector(Sz * (0.85 + H1), Sz, Sz);
				PutOne(X, Y + 0.018, Z, LITTER(H2 < 0.5 ? 0 : 1), 0.5 + H3 * 0.5, false);
			}
		}
	}
	// Freeform: what lies round a shrub's foot: up to three little heaps of pebbles and a dead twig, from the kinds'
	// own meshes. They are small, so the kinds' own fade (FLORA_WPO) keeps them within some 25 m of the eye.
	if (bNat && (bStones || K.Id == ES47FloraKind::Stalks))
	{
		const int32 Tries = bStones ? 3 : 1;
		const double Keep = bStones ? 0.6 : 0.5;
		for (const int32 Shrub : { 1, 2 })
		{
			const FKindCell& Cc = Cell(I, J, Shrub);
			for (int32 T = 0; T < Cc.N; ++T)
			{
				const float* M = Cc.M.GetData() + T * 16;
				const double Sh = FMath::Sqrt(double(M[0]) * M[0] + double(M[1]) * M[1] + double(M[2]) * M[2]);
				for (int32 Li = 0; Li < Tries; ++Li)
				{
					const double An = R() * UE_DOUBLE_PI * 2, Off = Sh * (0.25 + 0.6 * R()), Sb = R(), Ry = R(), L1 = R(), L2 = R(), Rank = R(), Hit = R();
					if (Sh < 0.45 || Hit >= Keep) continue;
					const double X = M[12] + FMath::Cos(An) * Off, Z = M[14] + FMath::Sin(An) * Off;
					// (not on a way, a plot, a pad or where the player walks)
					if (CoverAt(X, Z, Ki, 1.0) <= 0 || bSeamed || bWalked) continue;
					const double Y = Gy(X, Z);
					const FVector Col = Tint(L1, L2, Sb * 0.5);
					if (bStones)
					{
						const double H = 0.4, S = 0.12 + 0.18 * Sb, Sy = S * (0.7 + L1 * 0.6);
						const FVector Nrm = Normalize(FVector(-(Gy(X + H, Z) - Gy(X - H, Z)) / (2 * H), 1, -(Gy(X, Z + H) - Gy(X, Z - H)) / (2 * H)));
						Qt = QMul(QMul(QUnitVectors(UpV, Nrm), QAxisAngle(UpV, Ry * UE_DOUBLE_PI * 2)), S47Three::Euler((L1 - 0.5) * 0.4, 0, (L2 - 0.5) * 0.4));
						Sc = FVector(S, Sy, S);
						PutOne(X, Y - 0.025 * Sy, Z, Col, Rank, false);
					}
					else
					{
						// a dead twig, leaning well over
						const double S = 0.22 + 0.18 * Sb;
						Qt = S47Three::Euler((L1 - 0.5) * 0.9, Ry * UE_DOUBLE_PI * 2, (L2 - 0.5) * 0.9, TEXT("YXZ"));
						Sc = FVector(S, S, S);
						PutOne(X, Y - 0.035 * S, Z, Col, Rank, false);
					}
				}
			}
		}
	}
	// Freeform: the dead creosote, in Mesquite's meshes: the creosote's own candidates of this cell over again, from the same
	// random numbers, so a bush is live or dead and never both. Ranked 1 and up: Fill thins them as the creosote, not as the mesquite.
	if (bNat && K.Id == ES47FloraKind::Mesquite && bDesertOn)
	{
		const FKind& Cr = KindFor(true, CREOSOTE_KI);
		FMulberry Rc(Hash4(I, J, CREOSOTE_KI, Spec.Seed));
		for (int32 T = 0; T < 9; ++T)
		{
			const double U = Rc(), V = Rc(), A = Rc(), B = Rc(), CA = Rc(), CB = Rc(), Ry = Rc(), L1 = Rc(), L2 = Rc(), Rank = Rc(), N1 = Rc(), N2 = Rc(), N3 = Rc();
			(void)CA; (void)CB; (void)N2; (void)N3;
			double X = 0, Z = 0;
			CreosoteSpot(I, J, T, U, V, Spec.Seed, X, Z);
			const double Noise = NoiseAt(X, Z, CREOSOTE_KI);
			if (N1 >= DEAD_CREOSOTE_SHARE || A * Cr.Most >= Noise * 4.2) continue;
			if (A * Cr.Most >= FMath::Min(CoverAt(X, Z, CREOSOTE_KI, Noise), Cr.Most) || bWalked) continue;
			const double S = DEAD_SIZE[0] + B * (DEAD_SIZE[1] - DEAD_SIZE[0]), Spread = DESERT_PARTS[DEAD_PART].Radius * S;
			// (all of it off every way, plot, pad and walk, as a desert plant's)
			bool bClear = true;
			for (int32 Round = 0; Round < 8 && bClear; ++Round)
			{
				const double Turn = Round * UE_DOUBLE_PI / 4;
				bClear = CoverAt(X + FMath::Cos(Turn) * Spread, Z + FMath::Sin(Turn) * Spread, CREOSOTE_KI, 1.0) > 0 && !bWalked && !bSeamed;
			}
			if (!bClear) continue;
			PartNow = uint8(DEAD_PART);
			Qt = S47Three::Euler((L1 - 0.5) * 0.16, Ry * UE_DOUBLE_PI * 2, (L2 - 0.5) * 0.16, TEXT("YXZ"));
			Sc = FVector(S, S * (0.94 + L1 * 0.12), S);
			PutOne(X, Gy(X, Z) - 0.035 * S, Z, DEAD_TWIGS(L2 < 0.5 ? 0 : 1), 1 + Rank, false);
		}
	}
	Algo::StableSort(Out, [](const FOut& P, const FOut& Qo) { return P.Rank < Qo.Rank; });
	FKindCell Made;
	Made.N = Out.Num();
	Made.M.SetNumUninitialized(Out.Num() * 16); Made.C.SetNumUninitialized(Out.Num() * 3); Made.Rank.SetNumUninitialized(Out.Num());
	bool bAnySeam = false;
	for (const FOut& O : Out) bAnySeam |= O.bSeam;
	if (bAnySeam) Made.Seam.SetNumZeroed(Out.Num());
	const bool bParted = bDesert || bCreosoteModel;
	if (bParted) Made.Part.SetNumUninitialized(Out.Num());
	for (int32 T = 0; T < Out.Num(); ++T)
	{
		FMemory::Memcpy(Made.M.GetData() + T * 16, Out[T].M, sizeof(float) * 16);
		FMemory::Memcpy(Made.C.GetData() + T * 3, Out[T].Cl, sizeof(float) * 3);
		Made.Rank[T] = float(Out[T].Rank);
		if (bAnySeam && Out[T].bSeam) Made.Seam[T] = 1;
		if (bParted) Made.Part[T] = Out[T].Part;
	}
	return Made;
}

bool FS47Flora::Shown() const
{
	AS47Web* W = Web.Get();
	return W && Group >= 0 && FS47Obj(W, Group).Shown();
}

void FS47Flora::Follow(const FVector& CamPos, bool bLow)
{
	if (!Shown()) { DropShadows(); return; }
	if (Spec.bNatural && !bRockTried) EnsureRocks();
	if (Spec.bNatural && !bDesertTried) EnsureDesert();
	if (Spec.bNatural && !bCreosoteTried) EnsureCreosote();
	AS47Web* W = Web.Get();
	const FS47Obj G(W, Group);
	const FVector P = G.WorldToLocal(CamPos);
	const int32 NewQ = bLow ? 1 : 0;
	const double Moved = FMath::Sqrt(FMath::Square(P.X - At.X) + FMath::Square(P.Z - At.Y));
	if (!bDirty && NewQ == Q && Moved < STEP)
	{
		// the cells further out are made a little each picture; the ones a fill had to leave out are
		// put in once they are all made
		if (Ahead(At.X, At.Y) && bPending) Fill(At.X, At.Y, Q == 1);
		return;
	}
	// after a jump (a new area, a place out of a save, the first picture) all is made at once
	Fill(P.X, P.Z, bLow, !(Moved <= JUMP));
	At = FVector2D(P.X, P.Z); Q = NewQ; bDirty = false;
	Next = FNext();
}

bool FS47Flora::Ahead(double Px, double Pz)
{
	// Make the cells all round the camera out past each kind's reach, nearest first, for at most
	// AHEAD_MS in one picture; whether they are all made. Moving then only copies.
	if (Next.Kind >= NK) return true;
	const double T0 = FPlatformTime::Seconds();
	const int32 Ci = int32(FMath::FloorToDouble(Px / C)), Cj = int32(FMath::FloorToDouble(Pz / C));
	const TArray<FOff>& OFF = OFFSETS();
	for (; Next.Kind < NK; Next.Kind++, Next.At = 0)
	{
		const int32 Ki = Next.Kind;
		if (Ki >= ROCK_KI && !Spec.bNatural) continue;   // (the web version's rules have no rocks and no desert plants)
		if (Ki >= DESERT_KI && !bDesertOn) continue;
		const double R = KindFor(Spec.bNatural, Ki).Reach + 3 * STEP;
		for (; Next.At < OFF.Num(); Next.At++)
		{
			const FOff& O = OFF[Next.At];
			if (O.Dist > R + C) break;
			const TArray<TOptional<FKindCell>>* Cs = Cells.Find(Key(Ci + O.Dx, Cj + O.Dz));
			if (Cs && (*Cs)[Ki].IsSet()) continue;
			Cell(Ci + O.Dx, Cj + O.Dz, Ki);
			if ((FPlatformTime::Seconds() - T0) * 1000 > AHEAD_MS) return false;
		}
	}
	return true;
}

void FS47Flora::Fill(double Px, double Pz, bool bLow, bool bAll)
{
	const int32 Ci = int32(FMath::FloorToDouble(Px / C)), Cj = int32(FMath::FloorToDouble(Pz / C));
	// Unreal-only: do not upload instances already collapsed by FLORA_WPO. Keep one
	// refill step outside its fade so approaching plants enter while still invisible.
	// The distance comparison below is valid for the authored, level unit-scale patches.
	// Preserve the original fill for any future tilted/scaled patch.
	const FS47WebNode* GroupNode = Web.IsValid() ? Web->Node(Group) : nullptr;
	const USceneComponent* GroupComp = GroupNode ? GroupNode->Comp.Get() : nullptr;
	const bool bTrimInvisible = GroupComp && GroupComp->GetComponentScale().Equals(FVector::OneVector, 1e-5) &&
		GroupComp->GetUpVector().Equals(FVector::UpVector, 1e-5);
	bPending = false;
	FillX = Px; FillZ = Pz; bShadowNear = !bLow;
	const TArray<FOff>& OFF = OFFSETS();
	// Unreal-only: retain a bounded circle around the eye. The renderer handles
	// frustum/occlusion culling; a head turn must not replace whole plant batches.
	// Existing per-kind reaches, distance fades and instance caps still bound the work.
	for (int32 Ki = 0; Ki < NK; ++Ki)
	{
		const FKind& K = KindFor(Spec.bNatural, Ki);
		const double Keep = bLow ? K.Low : 1;
		const bool bFar = !bLow && K.bFar;
		// Low: none of the far ring, and nearer
		const double Reach = bLow ? (K.bFar ? K.Far[0] : K.Reach) * LOW_REACH : K.Reach;
		int32 N = 0, Candidates = 0, Omitted = 0;
		TArray<float> M, Cc;
		TArray<uint8> Pt;
		if (Keep > 0 && (Ki < ROCK_KI || (Spec.bNatural && (Ki == ROCK_KI || bDesertOn))))
		{
			const double R = Reach + STEP;
			// after a jump, everything out to the near reach at once (the far ring a picture or two later)
			const double Now = FMath::Max(SYNC, bAll ? (K.bFar ? K.Far[0] : R) : 0.0);
			M.Reserve(16 * 256); Cc.Reserve(3 * 256);
			for (int32 O = 0; O < OFF.Num(); ++O)
			{
				const FOff& Of = OFF[O];
				if (Of.Dist > R + C * 1.5) break;
				const int32 I = Ci + Of.Dx, J = Cj + Of.Dz;
				const double Nx = FMath::Max(I * C, FMath::Min(Px, (I + 1) * C)) - Px, Nz = FMath::Max(J * C, FMath::Min(Pz, (J + 1) * C)) - Pz;
				const double D2 = Nx * Nx + Nz * Nz;
				if (D2 > R * R) continue;
				// near ones are made now; further off, one not made yet comes in a picture or two later
				const FKindCell* Cell0 = nullptr;
				if (const TArray<TOptional<FKindCell>>* Cs = Cells.Find(Key(I, J)); Cs && (*Cs)[Ki].IsSet()) Cell0 = &*(*Cs)[Ki];
				else if (D2 < Now * Now) Cell0 = &Cell(I, J, Ki);
				if (!Cell0) { bPending = true; continue; }
				// past far[0] only the first share of it by rank, easing down over three cells
				const double Share = bFar && Of.Dist > K.Far[0] ? 1 + (K.Far[1] - 1) * FMath::Min(1.0, (Of.Dist - K.Far[0]) / (3 * C)) : 1;
				int32 Mn = Cell0->N;
				// Freeform's Mesquite holds the dead creosote too (rank 1 and up), and that goes thin where the creosote does
				const bool bDeadToo = Spec.bNatural && Ki == MESQUITE_KI;
				const FKind& Cr = KindFor(Spec.bNatural, CREOSOTE_KI);
				const double DeadShare = bDeadToo && !bLow && Of.Dist > Cr.Far[0] ? 1 + (Cr.Far[1] - 1) * FMath::Min(1.0, (Of.Dist - Cr.Far[0]) / (3 * C)) : 1;
				if (Keep * Share < 1 && !bDeadToo) { Mn = 0; while (Mn < Cell0->N && Cell0->Rank[Mn] < Keep * Share) Mn++; }
				for (int32 Instance = 0; Instance < Mn && N < K.Cap; ++Instance)
				{
					if (bDeadToo)
					{
						const double Rk = Cell0->Rank[Instance];
						if (Rk < 1 ? Rk >= Keep * Share : Rk - 1 >= Keep * DeadShare) continue;
					}
					const float* Matrix = Cell0->M.GetData() + Instance * 16;
					++Candidates;
					if (bTrimInvisible && Layers.IsValidIndex(Ki) && Layers[Ki].Mid.IsValid())
					{
						// Exact FLORA_WPO size/reach expression; cached procedural cells stay intact.
						const double Scale = FMath::Sqrt(FMath::Square(double(Matrix[0])) +
							FMath::Square(double(Matrix[1])) + FMath::Square(double(Matrix[2])));
						const double VisibleReach = Reach * FMath::Clamp(.05 + .75 * Scale, .3, 1.0) + STEP;
						if (FMath::Square(double(Matrix[12]) - Px) + FMath::Square(double(Matrix[14]) - Pz) >
							FMath::Square(VisibleReach)) { ++Omitted; continue; }
					}
					M.Append(Matrix, 16);
					Cc.Append(Cell0->C.GetData() + Instance * 3, 3);
					if (Cell0->Part.Num()) Pt.Add(Cell0->Part[Instance]);
					++N;
				}
				if (N >= K.Cap) break;
			}
		}
		if (Layers.IsValidIndex(Ki))
		{
			Layers[Ki].Reach = Reach;
			Layers[Ki].Candidates = Candidates;
			Layers[Ki].Omitted = Omitted;
			Layers[Ki].EndCull = bTrimInvisible ? FMath::CeilToInt((Reach + STEP + 4) * 100) : 0;
		}
		for (const FTestCopy& Tc : TestCopies) if (Tc.Ki == Ki) { M.Append(Tc.M, 16); Cc.Append(Tc.Cl, 3); Pt.Add(Tc.Part); ++N; }
		Put(Ki, M, Cc, Pt, N);
	}
	// forget the cells far behind
	if (Cells.Num() > 2500)
	{
		TArray<int64> Gone;
		for (const auto& KV : Cells)
		{
			const int32 I = int32(KV.Key / 80000) - 40000, J = int32(KV.Key % 80000) - 40000;
			if (FMath::Sqrt(FMath::Square((I + 0.5) * C - Px) + FMath::Square((J + 0.5) * C - Pz)) > MAX_R * 1.25) Gone.Add(KV.Key);
		}
		for (int64 K : Gone) Cells.Remove(K);
	}
}

TMap<ES47FloraKind, int32> FS47Flora::Drawn() const
{
	TMap<ES47FloraKind, int32> O;
	for (int32 Ki = 0; Ki < Layers.Num(); ++Ki) if (Layers[Ki].N > 0) O.Add(KINDS[Ki].Id, Layers[Ki].N);
	return O;
}

void FS47Flora::AuditDrawCounts(TArray<FString>& Lines) const
{
	for (int32 Ki = 0; Ki < Layers.Num(); ++Ki)
	{
		const FLayer& L = Layers[Ki];
		// (the desert plants only where they are in use; Story's lines are as they were)
		const bool bParts = UsesParts(Ki);
		if (Ki >= DESERT_KI && !bParts) continue;
		const UInstancedStaticMeshComponent* Mesh = bParts ? L.Parts[0].Mesh.Get() : L.Mesh.Get();
		int32 Start = 0, End = 0, Actual = Mesh ? Mesh->GetInstanceCount() : 0;
		if (Mesh) Mesh->GetCullDistances(Start, End);
		if (bParts) for (int32 Pi = 1; Pi < L.Parts.Num(); ++Pi) if (const UInstancedStaticMeshComponent* Other = L.Parts[Pi].Mesh.Get()) Actual += Other->GetInstanceCount();
		Lines.Add(FString::Printf(TEXT("flora %s kind=%d shown=%d eligible_examined=%d invisible_omitted=%d uploaded=%d actual_instances=%d cull_cm=%d,%d"),
			*Spec.Name, Ki, Shown(), L.Candidates, L.Omitted, L.N, Actual, Start, End));
	}
}

// ---------- Unreal ----------

namespace
{
	/** A shadow proxy (Freeform only fills it: Put): the same mesh and material, hidden but casting its shadow. */
	TWeakObjectPtr<UInstancedStaticMeshComponent> MakeShadow(AS47Web* W, UInstancedStaticMeshComponent* Src, UMaterialInstanceDynamic* Mid)
	{
		UInstancedStaticMeshComponent* P = NewObject<UInstancedStaticMeshComponent>(W);
		P->SetMobility(EComponentMobility::Movable);
		P->SetupAttachment(Src);
		P->SetStaticMesh(Src->GetStaticMesh());
		P->NumCustomDataFloats = 4;
		P->SetCastShadow(true);
		P->SetCastHiddenShadow(true);
		P->SetVisibleInRayTracing(false);
		P->bAffectDistanceFieldLighting = false;
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (Mid) P->SetMaterial(0, Mid);
		P->RegisterComponent();
		P->SetHiddenInGame(true);
		return TWeakObjectPtr<UInstancedStaticMeshComponent>(P);
	}
}

bool FS47Flora::Bind(AS47Web* W, int32 GroupNode)
{
	Web = W; Group = GroupNode;
	Layers.SetNum(NK);
	const FS47WebNode* G = W ? W->Node(GroupNode) : nullptr;
	if (!G) return false;
	// the meshes the patch has (flora.ts: one per kind, the second of a pair in the first's), by name
	const TPair<const TCHAR*, int32> Draws[] = { { TEXT("flora-grass+sage"), 0 }, { TEXT("flora-creosote+stalks"), 2 }, { TEXT("flora-succulent"), 4 }, { TEXT("flora-stones"), 5 }, { TEXT("flora-stain"), 6 } };
	int32 Bound = 0, ActivatedEmptyDraws = 0;
	for (int32 Ch : G->Children)
	{
		const FS47WebNode* N = W->Node(Ch);
		if (!N) continue;
		for (const auto& Dw : Draws)
		{
			if (N->Name != Dw.Key) continue;
			UInstancedStaticMeshComponent* A = Cast<UInstancedStaticMeshComponent>(N->Comp.Get());
			if (!A || !A->GetStaticMesh()) continue;
			const bool bRuntimeDraw = Spec.Name == TEXT("road") || Spec.Name == TEXT("saro") || Spec.Name == TEXT("station01");
			const bool bActivateDraw = bRuntimeDraw && !N->bVis;
			const bool bEmptyExport = A->GetInstanceCount() == 0;
			// the first kind: the export's own mesh, refilled from here (four floats a copy: its colour
			// and whether it is mirrored, a cholla); the plants cast no shadows and are hit by no ray
			A->ClearInstances();
			A->SetNumCustomDataFloats(4);
			A->SetCastShadow(false);
			A->SetVisibleInRayTracing(false);
			A->bAffectDistanceFieldLighting = false;
			A->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// Empty exported outdoor draws were hidden before the web's first fill.
			// Runtime now owns their population; keep the area's parent gate intact.
			if (bActivateDraw)
			{
				W->SetVisible(Ch, true);
				if (bEmptyExport) ++ActivatedEmptyDraws;
			}
			TArray<UMaterialInstanceDynamic*> Mids = W->NodeMids(Ch);
			UMaterialInstanceDynamic* Da = Mids.Num() ? Mids[0] : nullptr;
			if (Da) Da->SetScalarParameterValue(TEXT("UseInstanceColor"), 1.f);
			Layers[Dw.Value].Mesh = A; Layers[Dw.Value].Mid = Da;
			if (Dw.Value == 2) Layers[2].Shadow = MakeShadow(W, A, Da);
			++Bound;
			// the second of a pair: its own shape (export.js pairGeo) beside the first's, its own reach
			if (N->GeoB >= 0 && Dw.Value + 1 < NK)
			{
				UInstancedStaticMeshComponent* B = NewObject<UInstancedStaticMeshComponent>(W);
				B->SetMobility(EComponentMobility::Movable);
				B->SetupAttachment(A);
				B->SetStaticMesh(W->GeoMesh(N->GeoB));
				B->NumCustomDataFloats = 4;
				B->SetCastShadow(false);
				B->SetVisibleInRayTracing(false);
				B->bAffectDistanceFieldLighting = false;
				B->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				UMaterialInstanceDynamic* Db = Da ? W->CopyMid(Da) : nullptr;
				if (Db) B->SetMaterial(0, Db);
				B->RegisterComponent();
				B->SetVisibility(A->IsVisible());
				Layers[Dw.Value + 1].Mesh = B; Layers[Dw.Value + 1].Mid = Db;
				if (Dw.Value == 0) Layers[1].Shadow = MakeShadow(W, B, Db);
			}
		}
	}
	UE_LOG(LogS47Flora, Display, TEXT("S47: flora %s bound, %d meshes, runtime-enabled empty export draws=%d"), *Spec.Name, Bound, ActivatedEmptyDraws);
	return Bound > 0;
}

void FS47Flora::Put(int32 Ki, const TArray<float>& M, const TArray<float>& Cc, const TArray<uint8>& Pt, int32 N)
{
	if (!Layers.IsValidIndex(Ki)) return;
	FLayer& L = Layers[Ki];
	L.N = N;
	if (UMaterialInstanceDynamic* D = L.Mid.Get()) D->SetScalarParameterValue(TEXT("Reach"), float(L.Reach));
	if (UMaterialInstanceDynamic* D = L.PartMid.Get()) D->SetScalarParameterValue(TEXT("Reach"), float(L.Reach));
	// the creosote's models and its old cards never both: whichever is out of use (Story gets the cards back) is left with nothing
	const TArray<float> Nothing;
	if (!UsesParts(Ki))
	{
		PutMesh(L, L.Mesh.Get(), L.Shadow.Get(), Ki, M, Cc, N);
		for (const FLayerPart& Pa : L.Parts) PutMesh(L, Pa.Mesh.Get(), Pa.Shadow.Get(), Ki, Nothing, Nothing, 0);
		return;
	}
	if (Ki == CREOSOTE_KI) PutMesh(L, L.Mesh.Get(), L.Shadow.Get(), Ki, Nothing, Nothing, 0);
	// a species with variants: each copy to its variant's mesh
	const int32 FirstPart = Ki == CREOSOTE_KI ? 0 : DESERT_FIRST[Ki - DESERT_KI];
	for (int32 Pi = 0; Pi < L.Parts.Num(); ++Pi)
	{
		const int32 Wanted = FirstPart + Pi;
		TArray<float> Mp, Cp;
		int32 Np = 0;
		for (int32 k = 0; k < N; ++k)
		{
			if (!Pt.IsValidIndex(k) || Pt[k] != Wanted) continue;
			Mp.Append(M.GetData() + k * 16, 16);
			Cp.Append(Cc.GetData() + k * 3, 3);
			++Np;
		}
		PutMesh(L, L.Parts[Pi].Mesh.Get(), L.Parts[Pi].Shadow.Get(), Ki, Mp, Cp, Np);
	}
}

void FS47Flora::PutMesh(const FLayer& L, UInstancedStaticMeshComponent* I, UInstancedStaticMeshComponent* Sh, int32 Ki, const TArray<float>& M, const TArray<float>& Cc, int32 N)
{
	if (!I) return;
	// FLORA_WPO has fully collapsed every kind by Reach. Native culling also
	// discards its remaining vertex work outside that radius (plus refill/bounds margin).
	const int32 EndCull = L.EndCull;
	if (I->InstanceEndCullDistance != EndCull) I->SetCullDistances(EndCull, EndCull);
	TArray<FTransform> T;
	TArray<float> Cd;
	T.Reserve(N); Cd.Reserve(N * 4);
	// Freeform: the near creosote, sage and mesquite also cast shadows, from a proxy mesh holding only those near the eye
	const double ShadowR = Ki == 2 ? SHADOW_CREOSOTE : Ki == MESQUITE_KI ? SHADOW_MESQUITE : SHADOW_SAGE;
	const bool bShadows = Sh && Spec.bNatural && bShadowNear;
	TArray<FTransform> ShT;
	TArray<float> ShCd;
	for (int32 k = 0; k < N; ++k)
	{
		double E[16];
		for (int32 c = 0; c < 16; ++c) E[c] = M[k * 16 + c];
		T.Add(S47Three::MatToUE(E));
		// mirrored (a cholla, flora.ts succulentShape): the determinant of its 3x3 is negative
		const double Det = E[0] * (E[5] * E[10] - E[9] * E[6]) - E[4] * (E[1] * E[10] - E[9] * E[2]) + E[8] * (E[1] * E[6] - E[5] * E[2]);
		Cd.Add(Cc[k * 3]); Cd.Add(Cc[k * 3 + 1]); Cd.Add(Cc[k * 3 + 2]); Cd.Add(Det < 0 ? 1.f : 0.f);
		if (bShadows && FMath::Square(E[12] - FillX) + FMath::Square(E[14] - FillZ) < ShadowR * ShadowR)
		{
			ShT.Add(T.Last());
			ShCd.Add(Cd[k * 4]); ShCd.Add(Cd[k * 4 + 1]); ShCd.Add(Cd[k * 4 + 2]); ShCd.Add(Cd[k * 4 + 3]);
		}
	}
	if (I->GetInstanceCount() != N)
	{
		I->ClearInstances();
		if (N > 0) I->AddInstances(T, false, false, false);
	}
	else if (N > 0) I->BatchUpdateInstancesTransforms(0, T, false, false, true);
	if (N > 0) I->SetCustomData(0, N - 1, Cd, false); // (the last index, not past it)
	I->MarkRenderStateDirty();
	if (Sh && (Sh->GetInstanceCount() > 0 || ShT.Num() > 0))
	{
		Sh->ClearInstances();
		if (ShT.Num() > 0) { Sh->AddInstances(ShT, false, false, false); Sh->SetCustomData(0, ShT.Num() - 1, ShCd, false); }
		Sh->MarkRenderStateDirty();
	}
}

void FS47Flora::DropShadows()
{
	auto Drop = [](UInstancedStaticMeshComponent* Sh) { if (Sh && Sh->GetInstanceCount() > 0) { Sh->ClearInstances(); Sh->MarkRenderStateDirty(); } };
	for (FLayer& L : Layers)
	{
		Drop(L.Shadow.Get());
		for (const FLayerPart& Pa : L.Parts) Drop(Pa.Shadow.Get());
	}
}

void FS47Flora::SetNatural(bool bOn)
{
	if (Spec.bNatural == bOn) return;
	Spec.bNatural = bOn;
	// every cell is made again under the other rules, and the meshes refilled
	Cells.Empty(); Next = FNext(); bPending = false; bDirty = true;
	if (!bOn) DropShadows();
	if (Layers.IsValidIndex(STAIN_KI)) if (UMaterialInstanceDynamic* D = Layers[STAIN_KI].Mid.Get()) D->SetScalarParameterValue(TEXT("Opacity"), bOn ? FOOT_OPACITY : 1.f);
}

void FS47Flora::EnsureRocks()
{
	// Freeform's rocks are a real stone mesh, not the shape's lumps. Made when Freeform first needs them, so
	// Story has no such component at all; if the mesh is not there the rocks are simply left out.
	bRockTried = true;
	AS47Web* W = Web.Get();
	UInstancedStaticMeshComponent* Src = Layers.IsValidIndex(STONES_KI) ? Layers[STONES_KI].Mesh.Get() : nullptr;
	if (!W || !Src) return;
	UStaticMesh* Rock = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/CalicheStreet/NatureVegetation/SM_CN_Sandstone_A.SM_CN_Sandstone_A"));
	if (!Rock) { UE_LOG(LogS47Flora, Warning, TEXT("S47: flora %s: no rock mesh, Freeform's rocks left out"), *Spec.Name); return; }
	const FBox Box = Rock->GetBoundingBox();
	const double Across = FMath::Max(Box.Max.X - Box.Min.X, Box.Max.Y - Box.Min.Y);
	if (!Box.IsValid || Across < 1) return;
	// scale for a metre across; the foot lifted to the ground (the mesh's pivot may be anywhere) and a sixth of it sunk
	RockUnit = 100 / Across;
	RockFoot = (-Box.Min.Z - 0.15 * (Box.Max.Z - Box.Min.Z)) / 100;
	UInstancedStaticMeshComponent* P = NewObject<UInstancedStaticMeshComponent>(W);
	P->SetMobility(EComponentMobility::Movable);
	P->SetupAttachment(Src);
	P->SetStaticMesh(Rock);
	P->NumCustomDataFloats = 4;
	P->SetCastShadow(true);
	P->SetVisibleInRayTracing(false);
	P->bAffectDistanceFieldLighting = false;
	P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	P->RegisterComponent();
	P->SetVisibility(Src->IsVisible());
	Layers[ROCK_KI].Mesh = P;
	UE_LOG(LogS47Flora, Display, TEXT("S47: flora %s: rocks from %s, %.0f cm across as made"), *Spec.Name, *Rock->GetName(), Across);
}

int32 FS47Flora::DesertMeshes() { return DESERT_N; }

void FS47Flora::EnsureDesert()
{
	// Freeform's desert plants, made when Freeform first needs them, so Story has no such component at all. All eight or
	// none: with one missing (not imported, or a material without its master) the old succulent copies stay.
	bDesertTried = true;
	AS47Web* W = Web.Get();
	if (!W || !Layers.IsValidIndex(NK - 1)) return;
	UStaticMesh* Meshes[DESERT_N] = {};
	int32 Loaded = 0;
	for (int32 Pi = 0; Pi < DESERT_N; ++Pi)
	{
		const FString Path = FString::Printf(TEXT("/Game/DesertPlants/%s.%s"), DESERT_PARTS[Pi].Name, DESERT_PARTS[Pi].Name);
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		UMaterialInterface* Paint = Mesh ? Mesh->GetMaterial(0) : nullptr;
		const UMaterial* Master = Paint ? Paint->GetMaterial() : nullptr;
		if (Master && Master->GetName().StartsWith(TEXT("M_WebFlora"))) { Meshes[Pi] = Mesh; ++Loaded; }
	}
	UE_LOG(LogS47Flora, Display, TEXT("S47: desert plants %d of %d loaded (patch %s)"), Loaded, DESERT_N, *Spec.Name);
	// shown and hidden with the old succulents' draw, or the stones' if the patch has none
	UInstancedStaticMeshComponent* Src = Layers[SUCCULENT_KI].Mesh.Get();
	if (!Src) Src = Layers[STONES_KI].Mesh.Get();
	if (Loaded < DESERT_N || !Src)
	{
		UE_LOG(LogS47Flora, Warning, TEXT("S47: flora %s: no desert plants (Tools/desertplants/import_assets.py not run?), the old succulents stay"), *Spec.Name);
		return;
	}
	for (int32 Ki = DESERT_KI; Ki < NK; ++Ki)
	{
		FLayer& L = Layers[Ki];
		const int32 First = DESERT_FIRST[Ki - DESERT_KI], End = DESERT_FIRST[Ki - DESERT_KI + 1];
		// one material to a species: one set of what the flora's material takes at run time (Reach, WindDir)
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Meshes[First]->GetMaterial(0), W);
		// The atlases' small mips average leaf, flower and dry leaf together, pale: stay sharp however far the plant stands.
		if (D) D->SetForceMipLevelsToBeResident(true, true, -1.f);
		L.Mid = D;
		for (int32 Pi = First; Pi < End; ++Pi)
		{
			UInstancedStaticMeshComponent* P = NewObject<UInstancedStaticMeshComponent>(W);
			P->SetMobility(EComponentMobility::Movable);
			P->SetupAttachment(Src);
			P->SetStaticMesh(Meshes[Pi]);
			P->NumCustomDataFloats = 4;
			P->SetCastShadow(false);
			P->SetVisibleInRayTracing(false);
			P->bAffectDistanceFieldLighting = false;
			P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			P->SetMaterial(0, D);
			P->RegisterComponent();
			P->SetVisibility(Src->IsVisible());
			FLayerPart NewPart;
			NewPart.Mesh = P;
			// (the yucca as well: its blades shade each other, and without a shadow of its own it stands lit all through, pale, and off the ground)
			if (Ki == MESQUITE_KI || Ki == DESERT_KI) NewPart.Shadow = MakeShadow(W, P, D);
			L.Parts.Add(NewPart);
		}
	}
	bDesertOn = true;
}

bool FS47Flora::UsesParts(int32 Ki) const
{
	return Layers.IsValidIndex(Ki) && Layers[Ki].Parts.Num() > 0 && (Ki != CREOSOTE_KI || Spec.bNatural);
}

void FS47Flora::EnsureCreosote()
{
	// Freeform's creosote from its own models, by itself and apart from the other desert plants: made when Freeform first needs them, so Story has
	// no such component at all. All three or none: with one missing (not imported, or a material without its master) the old cards stay, the same copies.
	bCreosoteTried = true;
	AS47Web* W = Web.Get();
	UInstancedStaticMeshComponent* Src = Layers.IsValidIndex(CREOSOTE_KI) ? Layers[CREOSOTE_KI].Mesh.Get() : nullptr;
	if (!W || !Src) return;
	UStaticMesh* Meshes[CREOSOTE_N] = {};
	int32 Loaded = 0;
	for (int32 Pi = 0; Pi < CREOSOTE_N; ++Pi)
	{
		const FString Path = FString::Printf(TEXT("/Game/DesertPlants/%s.%s"), CREOSOTE_PARTS[Pi].Name, CREOSOTE_PARTS[Pi].Name);
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		UMaterialInterface* Paint = Mesh ? Mesh->GetMaterial(0) : nullptr;
		const UMaterial* Master = Paint ? Paint->GetMaterial() : nullptr;
		if (Master && Master->GetName().StartsWith(TEXT("M_WebFlora"))) { Meshes[Pi] = Mesh; ++Loaded; }
	}
	UE_LOG(LogS47Flora, Display, TEXT("S47: creosote models %d of %d loaded (patch %s)"), Loaded, CREOSOTE_N, *Spec.Name);
	if (Loaded < CREOSOTE_N) return;
	FLayer& L = Layers[CREOSOTE_KI];
	UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Meshes[0]->GetMaterial(0), W);
	// (as EnsureDesert: the atlas's small mips would average the leaves and the air between them)
	if (D) D->SetForceMipLevelsToBeResident(true, true, -1.f);
	L.PartMid = D;
	for (int32 Pi = 0; Pi < CREOSOTE_N; ++Pi)
	{
		UInstancedStaticMeshComponent* P = NewObject<UInstancedStaticMeshComponent>(W);
		P->SetMobility(EComponentMobility::Movable);
		P->SetupAttachment(Src);
		P->SetStaticMesh(Meshes[Pi]);
		P->NumCustomDataFloats = 4;
		P->SetCastShadow(false);
		P->SetVisibleInRayTracing(false);
		P->bAffectDistanceFieldLighting = false;
		P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		P->SetMaterial(0, D);
		P->RegisterComponent();
		P->SetVisibility(Src->IsVisible());
		FLayerPart NewPart;
		NewPart.Mesh = P;
		NewPart.Shadow = MakeShadow(W, P, D);   // (the same near ones as the cards' proxy: SHADOW_CREOSOTE)
		L.Parts.Add(NewPart);
	}
	bCreosoteOn = true;
	// (any cell made before this has the cards' colours and no variants)
	Cells.Empty(); Next = FNext(); bPending = false; bDirty = true;
}

int32 FS47Flora::TestDesertArc(const FVector& Eye, const FVector& Look, double Radius)
{
	AS47Web* W = Web.Get();
	if (!W || !bDesertOn) return 0;
	const FS47Obj G(W, Group);
	const FVector Ep = G.WorldToLocal(Eye), Lp = G.WorldToLocal(Look);
	const double Ahead0 = FMath::Atan2(Lp.Z - Ep.Z, Lp.X - Ep.X);
	const TFunction<double(double, double)>& Gd = Spec.Ground;
	TestCopies.Reset();
	for (int32 Ki = DESERT_KI; Ki < NK; ++Ki)
	{
		for (int32 Pi = DESERT_FIRST[Ki - DESERT_KI]; Pi < DESERT_FIRST[Ki - DESERT_KI + 1]; ++Pi)
		{
			// at scale 1, side by side over 84 degrees
			const double Ang = Ahead0 + FMath::DegreesToRadians((Pi - (DESERT_N - 1) / 2.0) * 12.0);
			const double X = Ep.X + FMath::Cos(Ang) * Radius, Z = Ep.Z + FMath::Sin(Ang) * Radius;
			double E[16];
			S47Three::Compose(FVector(X, (Gd ? Gd(X, Z) : 0.0) - 0.035, Z), QAxisAngle(UpV, Pi * 0.7), FVector(1, 1, 1), E);
			FTestCopy Tc;
			Tc.Ki = Ki; Tc.Part = uint8(Pi);
			for (int32 k = 0; k < 16; ++k) Tc.M[k] = float(E[k]);
			Tc.Cl[0] = Tc.Cl[1] = Tc.Cl[2] = 1.f;
			TestCopies.Add(Tc);
		}
	}
	// the creosote's models a little nearer, between the eye and the middle of that arc, in their own colours
	if (bCreosoteOn) for (int32 Pi = 0; Pi < CREOSOTE_N; ++Pi)
	{
		const double Ang = Ahead0 + FMath::DegreesToRadians((Pi - (CREOSOTE_N - 1) / 2.0) * 10.0), Near = Radius * 0.8;
		const double X = Ep.X + FMath::Cos(Ang) * Near, Z = Ep.Z + FMath::Sin(Ang) * Near;
		double E[16];
		S47Three::Compose(FVector(X, (Gd ? Gd(X, Z) : 0.0) - 0.035, Z), QAxisAngle(UpV, Pi * 0.7), FVector(1, 1, 1), E);
		FTestCopy Tc;
		Tc.Ki = CREOSOTE_KI; Tc.Part = uint8(Pi);
		for (int32 k = 0; k < 16; ++k) Tc.M[k] = float(E[k]);
		const FVector Cl = CREOSOTE_MODEL_PAL()[0];
		Tc.Cl[0] = float(Cl.X); Tc.Cl[1] = float(Cl.Y); Tc.Cl[2] = float(Cl.Z);
		TestCopies.Add(Tc);
	}
	bDirty = true;
	return TestCopies.Num();
}

void FS47Flora::SyncVisibility()
{
	for (int32 Ki : { 1, 3 })
	{
		UInstancedStaticMeshComponent* A = Layers.IsValidIndex(Ki - 1) ? Layers[Ki - 1].Mesh.Get() : nullptr;
		UInstancedStaticMeshComponent* B = Layers.IsValidIndex(Ki) ? Layers[Ki].Mesh.Get() : nullptr;
		if (A && B && B->IsVisible() != A->IsVisible()) B->SetVisibility(A->IsVisible());
	}
	// (Freeform's rocks are shown and hidden with the stones)
	UInstancedStaticMeshComponent* Stones = Layers.IsValidIndex(STONES_KI) ? Layers[STONES_KI].Mesh.Get() : nullptr;
	UInstancedStaticMeshComponent* Rocks = Layers.IsValidIndex(ROCK_KI) ? Layers[ROCK_KI].Mesh.Get() : nullptr;
	if (Stones && Rocks && Rocks->IsVisible() != Stones->IsVisible()) Rocks->SetVisibility(Stones->IsVisible());
	// (and the desert plants with what EnsureDesert put them under)
	if (bDesertOn)
	{
		const UInstancedStaticMeshComponent* Old = Layers[SUCCULENT_KI].Mesh.Get();
		if (!Old) Old = Stones;
		for (int32 Ki = DESERT_KI; Ki < NK && Old; ++Ki)
			for (const FLayerPart& Pa : Layers[Ki].Parts)
				if (UInstancedStaticMeshComponent* Pm = Pa.Mesh.Get()) if (Pm->IsVisible() != Old->IsVisible()) Pm->SetVisibility(Old->IsVisible());
	}
	// (the creosote's models with the old cards' draw)
	const UInstancedStaticMeshComponent* Cards = Layers.IsValidIndex(CREOSOTE_KI) ? Layers[CREOSOTE_KI].Mesh.Get() : nullptr;
	if (bCreosoteOn && Cards)
		for (const FLayerPart& Pa : Layers[CREOSOTE_KI].Parts)
			if (UInstancedStaticMeshComponent* Pm = Pa.Mesh.Get()) if (Pm->IsVisible() != Cards->IsVisible()) Pm->SetVisibility(Cards->IsVisible());
}

void FS47Flora::SetWind(const FVector2D& Dir)
{
	AS47Web* W = Web.Get();
	if (!W || Group < 0) return;
	// wind.dir is in the patch's terms (its shader turns it with the copies): into the world's, and Unreal's x, y
	const FVector Wd = FS47Obj(W, Group).WorldQuaternion().RotateVector(FVector(Dir.X, 0, Dir.Y));
	// Freeform: each kind sways its own amount (WIND_OF; the shader's offset follows the vector's length)
	for (int32 Ki = 0; Ki < Layers.Num(); ++Ki)
	{
		const double Sc = Spec.bNatural && Ki < NK ? WIND_OF[Ki] : 1.0;
		const FLinearColor V(float(Wd.X * Sc), float(Wd.Z * Sc), 0.f, 0.f);
		if (UMaterialInstanceDynamic* D = Layers[Ki].Mid.Get()) D->SetVectorParameterValue(TEXT("WindDir"), V);
		if (UMaterialInstanceDynamic* D = Layers[Ki].PartMid.Get()) D->SetVectorParameterValue(TEXT("WindDir"), V);
	}
}

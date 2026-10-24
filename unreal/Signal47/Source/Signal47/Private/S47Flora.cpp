#include "S47Flora.h"

#include "Algo/StableSort.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/PlatformTime.h"
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
	};
	constexpr int32 NK = 7;
	constexpr double CLUMP_K = 4.5;   // the drifts' noise raised to 2.5 averages about 1/4.5
	// NURSE: the grass and the stalks stand round the sage and the creosote
	const TArray<int32>& NurseOf(ES47FloraKind K)
	{
		static const TArray<int32> Shrubs = { 1, 2 }, None;
		return K == ES47FloraKind::Grass || K == ES47FloraKind::Stalks ? Shrubs : None;
	}
	constexpr double C = 8;             // cell size, metres
	constexpr double STEP = 8;          // the meshes are filled again when the camera has moved this far,
	constexpr double TURN = 0.26;       // or turned this far (radians): only what is in front of it is drawn,
	constexpr double CONE = 0.48;       // out to this much past the sides of the picture,
	constexpr double NEAR = 16;         // and all round it this close
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
	const TArray<FVector>& PAL(ES47FloraKind K)
	{
		static TArray<TArray<FVector>> P;
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
	/** Broom snakeweed, the commonest little shrub of the grassland: a sage copy, smaller, yellow-green. */
	const FVector& SNAKEWEED() { static const FVector V = Lin(0.98, 1.04, 0.66); return V; }
	constexpr double SNAKEWEED_SHARE = 0.32;
	/** The litter under a big tuft or a shrub (the stains): old leaves and shade, dark. */
	const FVector& LITTER(int32 I) { static const FVector V[2] = { Lin(0.22, 0.18, 0.14), Lin(0.27, 0.22, 0.17) }; return V[I]; }
	constexpr double LITTER_SHARE = 0.25;
	/** A pale caliche stone now and then among the dark ones. */
	const FVector& CALICHE() { static const FVector V = Lin(1.55, 1.48, 1.32); return V; }

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

double FS47Flora::NoiseAt(double X, double Z, int32 Ki) const
{
	const FKind& K = KINDS[Ki];
	double N = K.Patch > 1 ? 0.3 + 1.4 * VNoise(X, Z, K.Patch, Spec.Seed + int32(K.Patch)) : 1;
	// drifts a few metres across with bare dirt between (raised to a power: most of it thin, some thick)
	if (K.Clump > 0) N *= CLUMP_K * FMath::Pow(VNoise(X, Z, K.Clump, Spec.Seed + 911), 2.5);
	const double* Mx = Spec.Mix.Find(K.Id);
	return N * (Mx ? *Mx : 1);
}

double FS47Flora::CoverAt(double X, double Z, int32 Ki, double Noise)
{
	const FKind& K = KINDS[Ki];
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
	const FKind& K = KINDS[Ki];
	FMulberry R(Hash4(I, J, Ki, Spec.Seed));
	const double Want = K.Rate * C * C * K.Most;
	const int32 N = int32(FMath::FloorToDouble(Want)) + (R() < FMath::Fmod(Want, 1.0) ? 1 : 0);
	struct FOut { float M[16]; float Cl[3]; double Rank; bool bSeam; };
	TArray<FOut> Out;
	const TArray<FVector>& Pal = PAL(K.Id);
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
		O.Rank = Rank; O.bSeam = bSeam;
		Out.Add(O);
	};
	auto Tint = [&Pal](double CA, double CB, double F)
	{
		const int32 L = Pal.Num();
		const FVector& C0 = Pal[int32(FMath::FloorToDouble(CA * L)) % L];
		const FVector& C1 = Pal[int32(FMath::FloorToDouble(CB * L)) % L];
		return C0 + (C1 - C0) * F;
	};
	const bool bStones = K.Id == ES47FloraKind::Stones, bStain = K.Id == ES47FloraKind::Stain;
	for (int32 T = 0; T < N; ++T)
	{
		const double U = R(), V = R(), A = R(), B = R(), CA = R(), CB = R(), Ry = R(), L1 = R(), L2 = R(), Rank = R(), N1 = R(), N2 = R(), N3 = R();
		double X = (I + U) * C, Z = (J + V) * C;
		// most of the grass and the stalks stand 0.4 to 1.5 m from a shrub of the cell
		if (Nn && N1 < K.Nurse)
		{
			const int32 Qi = int32(FMath::FloorToDouble(N2 * Nn)) * 3;
			const double An = N3 * UE_DOUBLE_PI * 2, Dist = Nurse[Qi + 2] * 0.3 + 0.4 + V * 1.1;
			X = Nurse[Qi] + FMath::Cos(An) * Dist; Z = Nurse[Qi + 1] + FMath::Sin(An) * Dist;
		}
		const double Noise = NoiseAt(X, Z, Ki);
		if (A * K.Most >= Noise * 4.2) continue;   // (the cover is at most about that much, near a wall in a ditch)
		const double D = CoverAt(X, Z, Ki, Noise);
		if (A * K.Most >= FMath::Min(D, K.Most)) continue;
		// big where it is thick, small and mostly tiny in the open
		const double Dens = FMath::Min(1.0, D / K.Most);
		double S = K.Size[0] + FMath::Pow(B, 2.5 - 1.7 * Dens + (bStones ? 1.3 : 0)) * (K.Size[1] - K.Size[0]);
		if (bWalked) { if (K.Walk <= 0 || Fmod1(CB * 7.31) > K.Walk) continue; S = FMath::Min(S, K.WalkMax); }
		// in a seam tiny grass and pebbles; the gravel spilt off a way small
		if (bSeamed) S = K.Id == ES47FloraKind::Grass ? 0.25 + B * 0.2 : 0.3 + B * 0.2;
		else if (bSpilt) S = FMath::Min(S, 0.3 + B * 0.35);
		const double Y = Gy(X, Z);
		double Sy = S;
		FVector Col = Tint(CA, CB, L2 * 0.5);
		if (bStones || bStain)
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
				if (N1 < 0.06) Col = Col * CALICHE();
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
			PutOne(X, Y - 0.035 * S, Z, Col, Rank, bSeamed);
		}
	}
	// the litter under one in four of the big tufts and the shrubs: a small dark stain, ranked after
	// the stains (Kind.far: near the camera only)
	if (bStain)
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
	Algo::StableSort(Out, [](const FOut& P, const FOut& Qo) { return P.Rank < Qo.Rank; });
	FKindCell Made;
	Made.N = Out.Num();
	Made.M.SetNumUninitialized(Out.Num() * 16); Made.C.SetNumUninitialized(Out.Num() * 3); Made.Rank.SetNumUninitialized(Out.Num());
	bool bAnySeam = false;
	for (const FOut& O : Out) bAnySeam |= O.bSeam;
	if (bAnySeam) Made.Seam.SetNumZeroed(Out.Num());
	for (int32 T = 0; T < Out.Num(); ++T)
	{
		FMemory::Memcpy(Made.M.GetData() + T * 16, Out[T].M, sizeof(float) * 16);
		FMemory::Memcpy(Made.C.GetData() + T * 3, Out[T].Cl, sizeof(float) * 3);
		Made.Rank[T] = float(Out[T].Rank);
		if (bAnySeam && Out[T].bSeam) Made.Seam[T] = 1;
	}
	return Made;
}

bool FS47Flora::Shown() const
{
	AS47Web* W = Web.Get();
	return W && Group >= 0 && FS47Obj(W, Group).Shown();
}

void FS47Flora::Follow(const FVector& CamPos, const FVector& CamDir, double FovDeg, double Aspect, bool bLow)
{
	if (!Shown()) return;
	AS47Web* W = Web.Get();
	const FS47Obj G(W, Group);
	const FVector P = G.WorldToLocal(CamPos);
	// where it looks: what is behind it is not drawn (unless it looks down at its feet)
	const FVector D = G.WorldQuaternion().Inverse().RotateVector(CamDir);
	const double NewYaw = FMath::Atan2(D.X, D.Z);
	const double NewHalf = FMath::Sqrt(D.X * D.X + D.Z * D.Z) < 0.35 ? UE_DOUBLE_PI : FMath::Min(UE_DOUBLE_PI, FMath::Atan(FMath::Tan(FovDeg * UE_DOUBLE_PI / 360) * Aspect) + CONE);
	const bool bTurned = Half < UE_DOUBLE_PI && (FMath::Abs(FMath::Atan2(FMath::Sin(NewYaw - Yaw), FMath::Cos(NewYaw - Yaw))) > TURN || NewHalf > Half + 1e-3);
	const int32 NewQ = bLow ? 1 : 0;
	const double Moved = FMath::Sqrt(FMath::Square(P.X - At.X) + FMath::Square(P.Z - At.Y));
	if (!bDirty && NewQ == Q && !bTurned && Moved < STEP)
	{
		// the cells further out are made a little each picture; the ones a fill had to leave out are
		// put in once they are all made
		if (Ahead(At.X, At.Y) && bPending) Fill(At.X, At.Y, Q == 1);
		return;
	}
	Yaw = NewYaw; Half = NewHalf;
	// after a jump (a new area, a place out of a save, the first picture) all is made at once
	Fill(P.X, P.Z, bLow, !(Moved <= JUMP));
	At = FVector2D(P.X, P.Z); Q = NewQ; bDirty = false;
	Next = FNext();
}

bool FS47Flora::Ahead(double Px, double Pz)
{
	// Make the cells all round the camera out past each kind's reach, nearest first, for at most
	// AHEAD_MS in one picture; whether they are all made. Walking on or turning then only copies.
	if (Next.Kind >= NK) return true;
	const double T0 = FPlatformTime::Seconds();
	const int32 Ci = int32(FMath::FloorToDouble(Px / C)), Cj = int32(FMath::FloorToDouble(Pz / C));
	const TArray<FOff>& OFF = OFFSETS();
	for (; Next.Kind < NK; Next.Kind++, Next.At = 0)
	{
		const int32 Ki = Next.Kind;
		const double R = KINDS[Ki].Reach + 3 * STEP;
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
	const double Cr = C * 0.71;
	bPending = false;
	const TArray<FOff>& OFF = OFFSETS();
	// a cell is in front when its centre is within the cone, widened by the cell's own size: once
	// for each place in OFFSETS, for all the kinds (-1 not worked out yet, 0 behind, 1 in front)
	TArray<int8> Seen;
	Seen.Init(-1, OFF.Num());
	auto Front = [&](int32 O, int32 I, int32 J)
	{
		if (Half >= UE_DOUBLE_PI) return true;
		if (Seen[O] < 0)
		{
			const double Vx = (I + 0.5) * C - Px, Vz = (J + 0.5) * C - Pz, Dd = FMath::Sqrt(Vx * Vx + Vz * Vz), A = FMath::Atan2(Vx, Vz) - Yaw;
			Seen[O] = Dd < NEAR + Cr || FMath::Abs(FMath::Atan2(FMath::Sin(A), FMath::Cos(A))) <= Half + FMath::Asin(FMath::Min(1.0, Cr / Dd)) ? 1 : 0;
		}
		return Seen[O] == 1;
	};
	for (int32 Ki = 0; Ki < NK; ++Ki)
	{
		const FKind& K = KINDS[Ki];
		const double Keep = bLow ? K.Low : 1;
		const bool bFar = !bLow && K.bFar;
		// Low: none of the far ring, and nearer
		const double Reach = bLow ? (K.bFar ? K.Far[0] : K.Reach) * LOW_REACH : K.Reach;
		int32 N = 0;
		TArray<float> M, Cc;
		if (Keep > 0)
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
				if (D2 > R * R || !Front(O, I, J)) continue;
				// near ones are made now; further off, one not made yet comes in a picture or two later
				const FKindCell* Cell0 = nullptr;
				if (const TArray<TOptional<FKindCell>>* Cs = Cells.Find(Key(I, J)); Cs && (*Cs)[Ki].IsSet()) Cell0 = &*(*Cs)[Ki];
				else if (D2 < Now * Now) Cell0 = &Cell(I, J, Ki);
				if (!Cell0) { bPending = true; continue; }
				// past far[0] only the first share of it by rank, easing down over three cells
				const double Share = bFar && Of.Dist > K.Far[0] ? 1 + (K.Far[1] - 1) * FMath::Min(1.0, (Of.Dist - K.Far[0]) / (3 * C)) : 1;
				int32 Mn = Cell0->N;
				if (Keep * Share < 1) { Mn = 0; while (Mn < Cell0->N && Cell0->Rank[Mn] < Keep * Share) Mn++; }
				Mn = FMath::Min(Mn, K.Cap - N);
				if (Mn <= 0) continue;
				M.Append(Cell0->M.GetData(), Mn * 16);
				Cc.Append(Cell0->C.GetData(), Mn * 3);
				N += Mn;
				if (N >= K.Cap) break;
			}
		}
		if (Layers.IsValidIndex(Ki)) Layers[Ki].Reach = Reach;
		Put(Ki, M, Cc, N);
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

// ---------- Unreal ----------

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
			const bool bRuntimeDraw = Spec.Name == TEXT("road") || Spec.Name == TEXT("saro");
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
			// Empty exported road draws were hidden before the web's first fill.
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
			}
		}
	}
	UE_LOG(LogS47Flora, Display, TEXT("S47: flora %s bound, %d meshes, runtime-enabled empty export draws=%d"), *Spec.Name, Bound, ActivatedEmptyDraws);
	return Bound > 0;
}

void FS47Flora::Put(int32 Ki, const TArray<float>& M, const TArray<float>& Cc, int32 N)
{
	if (!Layers.IsValidIndex(Ki)) return;
	FLayer& L = Layers[Ki];
	L.N = N;
	UInstancedStaticMeshComponent* I = L.Mesh.Get();
	if (!I) return;
	if (UMaterialInstanceDynamic* D = L.Mid.Get()) D->SetScalarParameterValue(TEXT("Reach"), float(L.Reach));
	TArray<FTransform> T;
	TArray<float> Cd;
	T.Reserve(N); Cd.Reserve(N * 4);
	for (int32 k = 0; k < N; ++k)
	{
		double E[16];
		for (int32 c = 0; c < 16; ++c) E[c] = M[k * 16 + c];
		T.Add(S47Three::MatToUE(E));
		// mirrored (a cholla, flora.ts succulentShape): the determinant of its 3x3 is negative
		const double Det = E[0] * (E[5] * E[10] - E[9] * E[6]) - E[4] * (E[1] * E[10] - E[9] * E[2]) + E[8] * (E[1] * E[6] - E[5] * E[2]);
		Cd.Add(Cc[k * 3]); Cd.Add(Cc[k * 3 + 1]); Cd.Add(Cc[k * 3 + 2]); Cd.Add(Det < 0 ? 1.f : 0.f);
	}
	if (I->GetInstanceCount() != N)
	{
		I->ClearInstances();
		if (N > 0) I->AddInstances(T, false, false, false);
	}
	else if (N > 0) I->BatchUpdateInstancesTransforms(0, T, false, false, true);
	if (N > 0) I->SetCustomData(0, N - 1, Cd, false); // (the last index, not past it)
	I->MarkRenderStateDirty();
}

void FS47Flora::SyncVisibility()
{
	for (int32 Ki : { 1, 3 })
	{
		UInstancedStaticMeshComponent* A = Layers.IsValidIndex(Ki - 1) ? Layers[Ki - 1].Mesh.Get() : nullptr;
		UInstancedStaticMeshComponent* B = Layers.IsValidIndex(Ki) ? Layers[Ki].Mesh.Get() : nullptr;
		if (A && B && B->IsVisible() != A->IsVisible()) B->SetVisibility(A->IsVisible());
	}
}

void FS47Flora::SetWind(const FVector2D& Dir)
{
	AS47Web* W = Web.Get();
	if (!W || Group < 0) return;
	// wind.dir is in the patch's terms (its shader turns it with the copies): into the world's, and Unreal's x, y
	const FVector Wd = FS47Obj(W, Group).WorldQuaternion().RotateVector(FVector(Dir.X, 0, Dir.Y));
	const FLinearColor V(float(Wd.X), float(Wd.Z), 0.f, 0.f);
	for (const FLayer& L : Layers) if (UMaterialInstanceDynamic* D = L.Mid.Get()) D->SetVectorParameterValue(TEXT("WindDir"), V);
}

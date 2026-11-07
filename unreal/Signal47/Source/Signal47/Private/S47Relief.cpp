#include "S47Relief.h"
#include "S47Geo.h"
#include "S47GroundCover.h"
#include "S47RoadTerrain.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include <limits>

DEFINE_LOG_CATEGORY_STATIC(LogS47Relief, Log, All);

namespace
{
	struct FReliefTable
	{
		TArray<int16> Cells;
		double OriginX = 0, OriginZ = 0, InvCell = 1, ScaleM = 0;
		int32 Width = 0, Height = 0;
		bool bValid = false;
	};
	FReliefTable ReliefTable;
	bool bReliefLoaded = false, bReliefActive = false;

	// Tools/freeformterrain/build_range.py foot_table: the range over the road's land, one byte a cell.
	struct FFootTable
	{
		TArray<uint8> Cells;
		double OriginX = 0, OriginZ = 0, InvCell = 1, ScaleM = 0, BiasM = 0;
		int32 Width = 0, Height = 0;
		bool bValid = false;
	};
	FFootTable FootTable;
	constexpr double PastTheTable = 100;   // the hills go on past its edge

	double SampleFoot(const FFootTable& T, double X, double Z)
	{
		if (!T.bValid) return 0;
		const double U = (X - T.OriginX) * T.InvCell, V = (Z - T.OriginZ) * T.InvCell;
		if (!(U >= 0 && V >= 0 && U <= T.Width - 1 && V <= T.Height - 1)) return PastTheTable;
		const int32 I = FMath::Min(int32(U), T.Width - 2), J = FMath::Min(int32(V), T.Height - 2);
		const double Fx = U - I, Fz = V - J;
		const uint8* Row = T.Cells.GetData() + int64(J) * T.Width + I;
		const double Near = Row[0] + (double(Row[1]) - Row[0]) * Fx, Far = Row[T.Width] + (double(Row[T.Width + 1]) - Row[T.Width]) * Fx;
		return (Near + (Far - Near) * Fz) * T.ScaleM + T.BiasM;
	}

	double SampleTable(const FReliefTable& T, double X, double Z)
	{
		if (!T.bValid) return 0;
		const double U = (X - T.OriginX) * T.InvCell, V = (Z - T.OriginZ) * T.InvCell;
		// (written so that NaN lands outside as well)
		if (!(U >= 0 && V >= 0 && U <= T.Width - 1 && V <= T.Height - 1)) return 0;
		const int32 I = FMath::Min(int32(U), T.Width - 2), J = FMath::Min(int32(V), T.Height - 2);
		const double Fx = U - I, Fz = V - J;
		const int16* Row = T.Cells.GetData() + int64(J) * T.Width + I;
		const double A = Row[0], B = Row[1], C = Row[T.Width], D = Row[T.Width + 1];
		const double Near = A + (B - A) * Fx, Far = C + (D - C) * Fx;
		return (Near + (Far - Near) * Fz) * T.ScaleM;
	}

	uint32 ReliefCrc32(const uint8* Data, int64 Num)
	{
		// zlib's, which is what Python's zlib.crc32 gives. Computed here so its value does not depend on an engine helper.
		static const TArray<uint32> Table = []
		{
			TArray<uint32> Out; Out.SetNumUninitialized(256);
			for (uint32 N = 0; N < 256; ++N) { uint32 C = N; for (int32 K = 0; K < 8; ++K) C = C & 1 ? 0xEDB88320u ^ (C >> 1) : C >> 1; Out[N] = C; }
			return Out;
		}();
		uint32 Crc = 0xFFFFFFFFu;
		for (int64 I = 0; I < Num; ++I) Crc = Table[(Crc ^ Data[I]) & 0xFF] ^ (Crc >> 8);
		return Crc ^ 0xFFFFFFFFu;
	}

	FString ReliefSha256(const uint8* Data, int64 Num)
	{
		// FIPS 180-4. hashlib.sha256 is the usual thing for a Python tool to write; the engine has no plain SHA-256 in Core.
		static const uint32 K[64] = {
			0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
			0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
			0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
			0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };
		uint32 H[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
		const auto Rotr = [](uint32 V, int32 N) { return (V >> N) | (V << (32 - N)); };
		const auto Block = [&](const uint8* P)
		{
			uint32 W[64];
			for (int32 I = 0; I < 16; ++I) W[I] = (uint32(P[I * 4]) << 24) | (uint32(P[I * 4 + 1]) << 16) | (uint32(P[I * 4 + 2]) << 8) | uint32(P[I * 4 + 3]);
			for (int32 I = 16; I < 64; ++I)
			{
				const uint32 S0 = Rotr(W[I - 15], 7) ^ Rotr(W[I - 15], 18) ^ (W[I - 15] >> 3), S1 = Rotr(W[I - 2], 17) ^ Rotr(W[I - 2], 19) ^ (W[I - 2] >> 10);
				W[I] = W[I - 16] + S0 + W[I - 7] + S1;
			}
			uint32 V[8]; FMemory::Memcpy(V, H, sizeof(V));
			for (int32 I = 0; I < 64; ++I)
			{
				const uint32 S1 = Rotr(V[4], 6) ^ Rotr(V[4], 11) ^ Rotr(V[4], 25), Ch = (V[4] & V[5]) ^ (~V[4] & V[6]);
				const uint32 T1 = V[7] + S1 + Ch + K[I] + W[I];
				const uint32 S0 = Rotr(V[0], 2) ^ Rotr(V[0], 13) ^ Rotr(V[0], 22), Maj = (V[0] & V[1]) ^ (V[0] & V[2]) ^ (V[1] & V[2]);
				const uint32 T2 = S0 + Maj;
				V[7] = V[6]; V[6] = V[5]; V[5] = V[4]; V[4] = V[3] + T1; V[3] = V[2]; V[2] = V[1]; V[1] = V[0]; V[0] = T1 + T2;
			}
			for (int32 I = 0; I < 8; ++I) H[I] += V[I];
		};
		int64 Done = 0;
		for (; Done + 64 <= Num; Done += 64) Block(Data + Done);
		uint8 Tail[128] = { 0 };
		const int32 Rest = int32(Num - Done);
		if (Rest > 0) FMemory::Memcpy(Tail, Data + Done, Rest);
		Tail[Rest] = 0x80;
		const int32 TailLen = Rest < 56 ? 64 : 128;
		const uint64 Bits = uint64(Num) * 8;
		for (int32 I = 0; I < 8; ++I) Tail[TailLen - 1 - I] = uint8(Bits >> (8 * I));
		Block(Tail);
		if (TailLen == 128) Block(Tail + 64);
		FString Out;
		for (const uint32 Word : H) Out += FString::Printf(TEXT("%08x"), Word);
		return Out;
	}

	/** Every check the two files must pass. Out is left invalid unless all of them hold. */
	bool ParseReliefTable(const FString& Json, const TArray<uint8>& Bytes, FReliefTable& Out, FString& Why)
	{
		Out = FReliefTable();
		TSharedPtr<FJsonObject> Doc;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Doc) || !Doc.IsValid()) { Why = TEXT("relief.json does not parse"); return false; }
		double OriginX = 0, OriginZ = 0, CellM = 0, ScaleM = 0, W = 0, H = 0;
		if (!Doc->TryGetNumberField(TEXT("origin_x"), OriginX) || !Doc->TryGetNumberField(TEXT("origin_z"), OriginZ) || !Doc->TryGetNumberField(TEXT("cell_m"), CellM)
			|| !Doc->TryGetNumberField(TEXT("scale_m"), ScaleM) || !Doc->TryGetNumberField(TEXT("width"), W) || !Doc->TryGetNumberField(TEXT("height"), H))
		{ Why = TEXT("relief.json lacks a field of the contract"); return false; }
		if (!FMath::IsFinite(OriginX) || !FMath::IsFinite(OriginZ) || !FMath::IsFinite(CellM) || !FMath::IsFinite(ScaleM) || !FMath::IsFinite(W) || !FMath::IsFinite(H)
			|| CellM < .01 || CellM > 1000 || ScaleM <= 0 || ScaleM > 1 || W < 2 || H < 2 || W > 32768 || H > 32768 || W != FMath::FloorToDouble(W) || H != FMath::FloorToDouble(H))
		{ Why = TEXT("relief.json holds impossible values"); return false; }
		const int64 Count = int64(W) * int64(H);
		if (Count > (int64(1) << 29)) { Why = TEXT("relief.json is far too large"); return false; }
		if (int64(Bytes.Num()) != Count * 2) { Why = FString::Printf(TEXT("relief.bin is %d bytes, %lld expected"), Bytes.Num(), Count * 2); return false; }
		FString ShaText;
		if (Doc->TryGetStringField(TEXT("sha256"), ShaText) && !ShaText.Equals(ReliefSha256(Bytes.GetData(), Bytes.Num()), ESearchCase::IgnoreCase)) { Why = TEXT("relief.bin fails sha256"); return false; }
		const uint32 Crc = ReliefCrc32(Bytes.GetData(), Bytes.Num());
		// (a number or a hex string: TryGetStringField would also turn a number into text, so go by its type)
		if (const TSharedPtr<FJsonValue>* CrcValue = Doc->Values.Find(TEXT("crc32")))
		{
			const bool bMatch = (*CrcValue)->Type == EJson::String
				? (*CrcValue)->AsString().Replace(TEXT("0x"), TEXT("")).Equals(FString::Printf(TEXT("%08x"), Crc), ESearchCase::IgnoreCase)
				: (*CrcValue)->Type == EJson::Number && (*CrcValue)->AsNumber() == double(Crc);
			if (!bMatch) { Why = TEXT("relief.bin fails crc32"); return false; }
		}
		FReliefTable T;
		T.Cells.SetNumUninitialized(int32(Count));
		FMemory::Memcpy(T.Cells.GetData(), Bytes.GetData(), Bytes.Num());   // (int16 little endian, as every platform this game ships on)
		int32 Largest = 0;
		for (const int16 V : T.Cells) Largest = FMath::Max(Largest, FMath::Abs(int32(V)));
		// The land's whole roll is tens of metres; anything like this is a wrong scale or the wrong file.
		if (Largest * ScaleM > 120) { Why = FString::Printf(TEXT("relief.bin reaches %.1f m"), Largest * ScaleM); return false; }
		T.OriginX = OriginX; T.OriginZ = OriginZ; T.InvCell = 1 / CellM; T.ScaleM = ScaleM; T.Width = int32(W); T.Height = int32(H); T.bValid = true;
		// Off is 0 outside, so a live edge would be a cliff in the floor. Worth a line; not a reason to go flat.
		int32 Edge = 0;
		for (int32 I = 0; I < T.Width; ++I) Edge = FMath::Max3(Edge, FMath::Abs(int32(T.Cells[I])), FMath::Abs(int32(T.Cells[(T.Height - 1) * T.Width + I])));
		for (int32 J = 0; J < T.Height; ++J) Edge = FMath::Max3(Edge, FMath::Abs(int32(T.Cells[J * T.Width])), FMath::Abs(int32(T.Cells[J * T.Width + T.Width - 1])));
		if (Edge > 0) UE_LOG(LogS47Relief, Warning, TEXT("S47: relief table is not 0 at its edge (%.3f m): the floor steps there"), Edge * ScaleM);
		Out = MoveTemp(T);
		return true;
	}

	bool ParseFootTable(const FString& Json, TArray<uint8>& Bytes, FFootTable& Out, FString& Why)
	{
		TSharedPtr<FJsonObject> Doc;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Doc) || !Doc.IsValid()) { Why = TEXT("range_foot.json does not parse"); return false; }
		FFootTable T;
		double Cell = 0;
		if (!Doc->TryGetNumberField(TEXT("origin_x"), T.OriginX) || !Doc->TryGetNumberField(TEXT("origin_z"), T.OriginZ) || !Doc->TryGetNumberField(TEXT("cell_m"), Cell)
			|| !Doc->TryGetNumberField(TEXT("width"), T.Width) || !Doc->TryGetNumberField(TEXT("height"), T.Height) || !Doc->TryGetNumberField(TEXT("scale_m"), T.ScaleM) || !Doc->TryGetNumberField(TEXT("bias_m"), T.BiasM))
		{ Why = TEXT("range_foot.json lacks a field of the contract"); return false; }
		if (Cell <= 0 || T.ScaleM <= 0 || T.Width < 2 || T.Height < 2 || T.Width > 16384 || T.Height > 16384) { Why = TEXT("range_foot.json holds impossible values"); return false; }
		if (int64(Bytes.Num()) != int64(T.Width) * T.Height) { Why = FString::Printf(TEXT("range_foot.bin is %d bytes, %lld expected"), Bytes.Num(), int64(T.Width) * T.Height); return false; }
		FString ShaText;
		if (Doc->TryGetStringField(TEXT("sha256"), ShaText) && !ShaText.Equals(ReliefSha256(Bytes.GetData(), Bytes.Num()), ESearchCase::IgnoreCase)) { Why = TEXT("range_foot.bin fails sha256"); return false; }
		T.InvCell = 1 / Cell;
		T.Cells = MoveTemp(Bytes);
		T.bValid = true;
		Out = MoveTemp(T);
		return true;
	}

	void LoadReliefOnce()
	{
		if (bReliefLoaded) return;
		bReliefLoaded = true;
		const FString Dir = FPaths::ProjectContentDir() / TEXT("FreeformTerrain");
		FString Json, Why;
		TArray<uint8> Bytes;
		// Whatever becomes of the relief. Without this table the hills stop nothing and the log says so.
		{
			FString FootJson, FootWhy;
			TArray<uint8> FootBytes;
			if (!FFileHelper::LoadFileToString(FootJson, *(Dir / TEXT("range_foot.json"))) || !FFileHelper::LoadFileToArray(FootBytes, *(Dir / TEXT("range_foot.bin")), FILEREAD_Silent))
			{
				UE_LOG(LogS47Relief, Display, TEXT("S47: no table of the hills' foot: they stop nothing"));
			}
			else if (!ParseFootTable(FootJson, FootBytes, FootTable, FootWhy))
			{
				UE_LOG(LogS47Relief, Error, TEXT("S47: table of the hills' foot rejected: %s; they stop nothing"), *FootWhy);
			}
			else
			{
				UE_LOG(LogS47Relief, Display, TEXT("S47: table of the hills' foot %dx%d, %.2f m cells"), FootTable.Width, FootTable.Height, 1 / FootTable.InvCell);
			}
		}
		if (!FFileHelper::LoadFileToString(Json, *(Dir / TEXT("relief.json"))) || !FFileHelper::LoadFileToArray(Bytes, *(Dir / TEXT("relief.bin")), FILEREAD_Silent))
		{
			UE_LOG(LogS47Relief, Display, TEXT("S47: relief table not installed; the land stays as it was"));
			return;
		}
		if (!ParseReliefTable(Json, Bytes, ReliefTable, Why))
		{
			UE_LOG(LogS47Relief, Error, TEXT("S47: relief table rejected: %s; the land stays as it was"), *Why);
			return;
		}
		UE_LOG(LogS47Relief, Display, TEXT("S47: relief table %dx%d, %.2f m cells, %.4f m a step"), ReliefTable.Width, ReliefTable.Height, 1 / ReliefTable.InvCell, ReliefTable.ScaleM);
	}
}

namespace S47Relief
{
	double Off(double RoadX, double RoadZ) { return bReliefActive ? SampleTable(ReliefTable, RoadX, RoadZ) : 0; }
	double Sample(double RoadX, double RoadZ) { return SampleTable(ReliefTable, RoadX, RoadZ); }
	bool Ready() { LoadReliefOnce(); return ReliefTable.bValid; }
	void SetActive(bool bOn) { bReliefActive = bOn && Ready(); }
	bool Active() { return bReliefActive; }
	double HillAbove(double RoadX, double RoadZ) { return bReliefActive ? SampleFoot(FootTable, RoadX, RoadZ) : 0; }
	bool HillStops(double RoadX, double RoadZ, double Reach)
	{
		if (!bReliefActive || !FootTable.bValid) return false;
		for (const FVector2D& D : { FVector2D(0, 0), FVector2D(-Reach, -Reach), FVector2D(Reach, -Reach), FVector2D(-Reach, Reach), FVector2D(Reach, Reach) })
			if (SampleFoot(FootTable, RoadX + D.X, RoadZ + D.Y) > FootStopM) return true;
		return false;
	}
	FSuspend::FSuspend() : bWas(bReliefActive) { bReliefActive = false; }
	FSuspend::~FSuspend() { bReliefActive = bWas; }

	bool CheckZones(TArray<FString>& Lines)
	{
		if (!Ready()) { Lines.Add(TEXT("skip relief zones: no table")); return true; }
		bool bAll = true;
		const double Tol = .02, Step = FMath::Max(1., 1 / ReliefTable.InvCell);
		const auto Report = [&](const TCHAR* Name, double Largest, const TCHAR* Why)
		{
			const bool bGood = Largest <= Tol;
			bAll &= bGood;
			Lines.Add(FString::Printf(TEXT("%s relief zone %s: largest %.3f m (%s)"), bGood ? TEXT("PASS") : TEXT("FAIL"), Name, Largest, Why));
		};
		const auto Rect = [&](const TCHAR* Name, double X0, double X1, double Z0, double Z1, const TCHAR* Why)
		{
			double Largest = 0;
			for (double Z = Z0; Z <= Z1; Z += Step) for (double X = X0; X <= X1; X += Step) Largest = FMath::Max(Largest, FMath::Abs(Sample(X, Z)));
			Report(Name, Largest, Why);
		};
		using S47Geo::SARO_IN_ROAD;
		Rect(TEXT("SARO_PATCH"), S47Geo::SARO_PATCH.MinX, S47Geo::SARO_PATCH.MaxX, S47Geo::SARO_PATCH.MinZ, S47Geo::SARO_PATCH.MaxZ, TEXT("SARO area drives and walks SurfaceY over its own flat corridor meshes, nodes 953 and the verge"));
		Rect(TEXT("STATION_PATCH"), S47Geo::STATION_PATCH.MinX, S47Geo::STATION_PATCH.MaxX, S47Geo::STATION_PATCH.MinZ, S47Geo::STATION_PATCH.MaxZ, TEXT("the station area the same over node 1120"));
		Rect(TEXT("farm"), 27, 201, 130, 290, TEXT("the farm's pad, buildings, yard ground and road ribbon stand on SurfaceY as it was"));
		// Dishes reach x -230..210, z -420..-42 in SARO's terms; the graded array is about 60 m wider and 95 m further north.
		Rect(TEXT("array"), -290 + SARO_IN_ROAD.X, 270 + SARO_IN_ROAD.X, -515 + SARO_IN_ROAD.Z, SARO_IN_ROAD.Z, TEXT("the antennas stand on flat ground; S47Legs and SaroFloorAt say -0.6 here"));
		{
			double Largest = 0;
			for (double Z = -300; Z <= 300; Z += Step) for (double X = -300; X <= 300; X += Step)
				if (X * X + Z * Z <= 300 * 300) Largest = FMath::Max(Largest, FMath::Abs(Sample(X + SARO_IN_ROAD.X, Z + SARO_IN_ROAD.Z)));
			Report(TEXT("SARO site"), Largest, TEXT("SaroLand is flat within 300 m; WorldLife's haze and SaroFloorAt read -0.62"));
		}
		// SurfaceY's strips come before the grid and take no Off, so anything live at their outer edge is a step in the floor.
		{
			double Largest = 0;
			for (double Z = S47RoadTerrain::HWY.Z0; Z <= S47RoadTerrain::HWY.Z1; Z += Step) for (const double X : { -S47RoadTerrain::HWY.Shoulder, S47RoadTerrain::HWY.Shoulder }) Largest = FMath::Max(Largest, FMath::Abs(Sample(X, Z)));
			Report(TEXT("highway edge"), Largest, TEXT("the strip takes no Off"));
			Largest = 0;
			const S47RoadTerrain::FTrack& Track = S47RoadTerrain::Track();
			for (int32 I = 0; I < Track.Pts.Num(); ++I)
			{
				const double W = S47RoadTerrain::TrackHalf(Track.S[I]) + S47RoadTerrain::TRK.Soft;
				for (const double Dz : { -W, W }) for (const double Dx : { -W, W }) Largest = FMath::Max(Largest, FMath::Abs(Sample(Track.Pts[I].X + Dx, Track.Pts[I].Y + Dz)));
			}
			Report(TEXT("track edge"), Largest, TEXT("the strip takes no Off"));
			Largest = 0;
			for (const S47RoadTerrain::FStubPt& P : S47RoadTerrain::OldStub())
				for (const double Side : { -S47RoadTerrain::OLD.Shoulder, S47RoadTerrain::OLD.Shoulder }) Largest = FMath::Max(Largest, FMath::Abs(Sample(P.X - P.Tz * Side, P.Z + P.Tx * Side)));
			Report(TEXT("old road edge"), Largest, TEXT("the strip takes no Off"));
		}
		return bAll;
	}

	bool SelfTest(TArray<FString>& Lines)
	{
		bool bOk = true;
		const auto Check = [&](bool bPass, const FString& What) { Lines.Add(FString(bPass ? TEXT("PASS relief: ") : TEXT("FAIL relief: ")) + What); bOk &= bPass; };

		// Checksums against published vectors, since the two tools have to agree.
		{
			const uint8 Abc[3] = { 'a', 'b', 'c' };
			const uint8 Digits[9] = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
			Check(ReliefSha256(Abc, 3) == TEXT("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"), TEXT("sha256 of abc"));
			Check(ReliefCrc32(Digits, 9) == 0xCBF43926u, TEXT("crc32 of 123456789"));
		}

		// The whole test runs on a small made-up table, so it needs no map. What the game holds is put back at the end.
		FReliefTable Saved = MoveTemp(ReliefTable);
		const bool bSavedActive = bReliefActive, bSavedLoaded = bReliefLoaded;
		ReliefTable = FReliefTable(); bReliefActive = false; bReliefLoaded = true;

		// A ring 60 to 280 m round road (-600, 1100), open desert grid with no strip near it, and 0 in the middle and at the edges.
		const double CentreX = -600, CentreZ = 1100;
		FReliefTable Made;
		Made.Width = 61; Made.Height = 61; Made.OriginX = -900; Made.OriginZ = 800; Made.InvCell = .1; Made.ScaleM = .001; Made.bValid = true;
		Made.Cells.SetNumZeroed(Made.Width * Made.Height);
		for (int32 J = 0; J < Made.Height; ++J) for (int32 I = 0; I < Made.Width; ++I)
		{
			const double R = FVector2D(Made.OriginX + I * 10 - CentreX, Made.OriginZ + J * 10 - CentreZ).Size();
			Made.Cells[J * Made.Width + I] = int16(FMath::RoundToInt(1500 * FMath::SmoothStep(60., 120., R) * (1 - FMath::SmoothStep(200., 280., R))));
		}

		FRandomStream Random(47101);
		TArray<FVector2D> Points;
		for (int32 I = 0; I < 10000; ++I) Points.Add(FVector2D(Random.FRandRange(-2950.f, 2250.f), Random.FRandRange(-2650.f, 4350.f)));
		for (int32 I = 0; I < 2000; ++I) Points.Add(FVector2D(Random.FRandRange(-920.f, -280.f), Random.FRandRange(780.f, 1420.f)));
		TArray<double> Base;
		Base.Reserve(Points.Num());
		for (const FVector2D& P : Points) Base.Add(S47RoadTerrain::SurfaceY(P.X, P.Y));

		ReliefTable = Made;
		{
			int32 Bad = 0;
			for (int32 I = 0; I < Points.Num(); ++I) Bad += S47RoadTerrain::SurfaceY(Points[I].X, Points[I].Y) != Base[I];
			Check(Bad == 0, FString::Printf(TEXT("flag off: SurfaceY is the same to the bit at %d points with a table loaded (%d differ)"), Points.Num(), Bad));
		}
		bReliefActive = true;
		{
			int32 Masked = 0, Live = 0, BadMask = 0, BadLive = 0;
			for (int32 I = 0; I < Points.Num(); ++I)
			{
				const double Want = SampleTable(Made, Points[I].X, Points[I].Y), Got = S47RoadTerrain::SurfaceY(Points[I].X, Points[I].Y);
				if (Want == 0) { ++Masked; BadMask += Got != Base[I]; }
				else { ++Live; BadLive += FMath::Abs(Got - Base[I] - Want) > 1e-9; }
			}
			Check(BadMask == 0 && Masked > 5000, FString::Printf(TEXT("flag on: SurfaceY is unchanged at %d points in the mask or off the table (%d differ)"), Masked, BadMask));
			Check(BadLive == 0 && Live > 500, FString::Printf(TEXT("flag on: SurfaceY rises by the table at %d points (%d differ)"), Live, BadLive));
		}
		{
			// Across the whole table and over both of its edges, a quarter of a metre at a time: nothing steps.
			double Worst = 0, Before = 0;
			bool bFirst = true;
			for (double X = -950; X <= -250; X += .25)
			{
				const double Now = S47RoadTerrain::SurfaceY(X, CentreZ);
				bReliefActive = false;
				const double Flat = S47RoadTerrain::SurfaceY(X, CentreZ);
				bReliefActive = true;
				if (!bFirst) Worst = FMath::Max(Worst, FMath::Abs((Now - Flat) - Before));
				Before = Now - Flat; bFirst = false;
			}
			Check(Worst < .02, FString::Printf(TEXT("flag on: no step anywhere across the table (largest %.4f m in 0.25 m)"), Worst));
		}
		{
			// SARO's ring reads the same table a frame away: site = road + (-24, +300).
			int32 BadSaro = 0, BadMask = 0;
			for (int32 I = 0; I < 2000; ++I)
			{
				const double Rx = Random.FRandRange(-920.f, -280.f), Rz = Random.FRandRange(780.f, 1420.f);
				const double Sx = Rx - S47Geo::SARO_IN_ROAD.X, Sz = Rz - S47Geo::SARO_IN_ROAD.Z;
				const double Got = S47GroundCover::SaroLand(Sx, Sz);
				bReliefActive = false;
				const double Flat = S47GroundCover::SaroLand(Sx, Sz);
				bReliefActive = true;
				BadSaro += FMath::Abs(Got - Flat - SampleTable(Made, Rx, Rz)) > 1e-9;
			}
			// The site's flat ground stays where SaroFloorAt, S47Legs and the haze have it, as long as the table is 0 there.
			for (int32 I = 0; I < 2000; ++I)
			{
				const double Sx = Random.FRandRange(-300.f, 300.f), Sz = Random.FRandRange(-300.f, 300.f);
				const double Got = S47GroundCover::SaroLand(Sx, Sz);
				bReliefActive = false;
				const double Flat = S47GroundCover::SaroLand(Sx, Sz);
				bReliefActive = true;
				BadMask += Got != Flat;
			}
			Check(BadSaro == 0, FString::Printf(TEXT("SaroLand rises by the table in the road's terms (%d differ)"), BadSaro));
			Check(BadMask == 0, FString::Printf(TEXT("SaroLand is unchanged where the table is 0 (%d differ)"), BadMask));
		}
		{
			const double Node = SampleTable(Made, -710, 1100), NodeWant = Made.Cells[30 * 61 + 19] * .001;
			const double Mid = SampleTable(Made, -705, 1100), MidWant = (Made.Cells[30 * 61 + 19] + Made.Cells[30 * 61 + 20]) * .5 * .001;
			Check(FMath::Abs(Node - NodeWant) < 1e-9 && FMath::Abs(Mid - MidWant) < 1e-9, TEXT("bilinear: on a sample, and halfway to the next"));
			Check(SampleTable(Made, -901, 1100) == 0 && SampleTable(Made, -299, 1100) == 0 && SampleTable(Made, -600, 799) == 0 && SampleTable(Made, -600, 1401) == 0 && SampleTable(Made, std::numeric_limits<double>::quiet_NaN(), 1100) == 0, TEXT("0 off the table on all four sides, and for NaN"));
			Check(SampleTable(Made, -300, 1400) == 0, TEXT("the far corner reads without running past the table"));
		}
		{
			// A bad file is 0 everywhere and no crash.
			const FString Good = TEXT("{\"origin_x\":0,\"origin_z\":0,\"cell_m\":4,\"width\":3,\"height\":3,\"scale_m\":0.002}");
			TArray<uint8> Bytes; Bytes.SetNumZeroed(18);
			const int16 Twenty = 10000; FMemory::Memcpy(Bytes.GetData() + 8, &Twenty, 2);
			FReliefTable Parsed; FString Why;
			const bool bGood = ParseReliefTable(Good, Bytes, Parsed, Why);
			Check(bGood && FMath::Abs(SampleTable(Parsed, 4, 4) - 20) < 1e-9 && FMath::Abs(SampleTable(Parsed, 2, 4) - 10) < 1e-9 && SampleTable(Parsed, 0, 0) == 0, TEXT("a good pair of files reads little endian, in rows of x"));
			TArray<uint8> Short; Short.SetNumZeroed(17);
			TArray<uint8> Huge; Huge.SetNumZeroed(18); const int16 Top = 32767; FMemory::Memcpy(Huge.GetData() + 8, &Top, 2);
			const struct { const TCHAR* Name; FString Json; const TArray<uint8>* Data; } Cases[] = {
				{ TEXT("short file"), Good, &Short },
				{ TEXT("broken json"), TEXT("{\"origin_x\":"), &Bytes },
				{ TEXT("missing field"), TEXT("{\"origin_x\":0,\"origin_z\":0,\"cell_m\":4,\"width\":3,\"height\":3}"), &Bytes },
				{ TEXT("zero cell"), TEXT("{\"origin_x\":0,\"origin_z\":0,\"cell_m\":0,\"width\":3,\"height\":3,\"scale_m\":0.002}"), &Bytes },
				{ TEXT("negative width"), TEXT("{\"origin_x\":0,\"origin_z\":0,\"cell_m\":4,\"width\":-3,\"height\":-6,\"scale_m\":0.002}"), &Bytes },
				{ TEXT("wrong scale"), TEXT("{\"origin_x\":0,\"origin_z\":0,\"cell_m\":4,\"width\":3,\"height\":3,\"scale_m\":0.5}"), &Huge },
				{ TEXT("wrong sha256"), TEXT("{\"origin_x\":0,\"origin_z\":0,\"cell_m\":4,\"width\":3,\"height\":3,\"scale_m\":0.002,\"sha256\":\"00\"}"), &Bytes },
				{ TEXT("wrong crc32"), TEXT("{\"origin_x\":0,\"origin_z\":0,\"cell_m\":4,\"width\":3,\"height\":3,\"scale_m\":0.002,\"crc32\":1}"), &Bytes },
			};
			for (const auto& Case : Cases)
			{
				FReliefTable Bad; FString BadWhy;
				const bool bParsed = ParseReliefTable(Case.Json, *Case.Data, Bad, BadWhy);
				Check(!bParsed && !Bad.bValid && SampleTable(Bad, 0, 0) == 0, FString::Printf(TEXT("%s is refused and reads 0 (%s)"), Case.Name, *BadWhy));
			}
		}

		ReliefTable = MoveTemp(Saved); bReliefActive = bSavedActive; bReliefLoaded = bSavedLoaded;

		// The foot, on a made-up table too: 2 x 2 cells of 10 m, under the ground at one side and 8 m over it at the other.
		{
			FFootTable SavedFoot = MoveTemp(FootTable);
			const bool bWasActive = bReliefActive;
			const FString FootJson = TEXT("{\"origin_x\":100,\"origin_z\":200,\"cell_m\":10,\"width\":2,\"height\":2,\"scale_m\":0.125,\"bias_m\":-2}");
			TArray<uint8> FootBytes = { 0, 80, 0, 80 };
			FString FootWhy;
			Check(ParseFootTable(FootJson, FootBytes, FootTable, FootWhy), TEXT("a foot table parses"));
			bReliefActive = true;
			Check(FMath::IsNearlyEqual(HillAbove(100, 205), -2., 1e-6) && FMath::IsNearlyEqual(HillAbove(110, 205), 8., 1e-6) && FMath::IsNearlyEqual(HillAbove(105, 205), 3., 1e-6), TEXT("the foot reads across a cell"));
			Check(HillAbove(90, 205) == PastTheTable, TEXT("past the foot table the hills go on"));
			Check(!HillStops(100.5, 205, .1) && HillStops(109, 205, .1) && HillStops(100.5, 205, 8), TEXT("a footprint stops by whichever corner meets the foot"));
			bReliefActive = false;
			Check(HillAbove(110, 205) == 0 && !HillStops(109, 205, .1), TEXT("off, the hills stop nothing"));
			bReliefActive = bWasActive;
			FootTable = MoveTemp(SavedFoot);
		}
		// What it costs: this is asked a few times a frame.
		{
			const bool bWasActive = bReliefActive;
			bReliefActive = true;
			const double Started = FPlatformTime::Seconds();
			double Sum = 0;
			for (int32 I = 0; I < 200000; ++I) Sum += HillStops(-2900 + I * .025, 500 + I * .019, .35) ? 1 : 0;
			const double Took = FPlatformTime::Seconds() - Started;
			bReliefActive = bWasActive;
			Lines.Add(FString::Printf(TEXT("relief foot: %.0f ns a footprint (%d of 200000 stopped)"), Took / 200000 * 1e9, int32(Sum)));
		}

		// With the real table on disk, the places that depend on it being 0. Without it there is nothing to say.
		TArray<FString> Zones;
		const FString Dir = FPaths::ProjectContentDir() / TEXT("FreeformTerrain");
		if (FPaths::FileExists(Dir / TEXT("relief.bin")))
		{
			const bool bZones = Ready() ? CheckZones(Zones) : false;
			Lines.Append(Zones);
			Check(bZones, TEXT("the installed table passes its checks and is 0 where nothing follows it"));
		}
		else Lines.Add(TEXT("skip relief: no table installed, zones not read"));
		return bOk;
	}
}

#include "S47Three.h"

namespace S47Three
{
namespace
{
	// The swap of Y and Z (index map), applied on both sides of a three.js 3x3 block.
	constexpr int32 Pm[3] = { 0, 2, 1 };
}

FTransform MatToUE(const double* E)
{
	// three: A[r][c] = E[c * 4 + r], t = E[12..14]. Unreal: B = P A P, t' = 100 P t, and an
	// FMatrix holds row vectors, so M[i][j] = B[j][i].
	FMatrix M = FMatrix::Identity;
	for (int32 R = 0; R < 3; ++R)
	{
		for (int32 C = 0; C < 3; ++C)
		{
			const double A = E[Pm[C] * 4 + Pm[R]]; // B[R][C] = A[p(R)][p(C)]
			M.M[C][R] = A;
		}
	}
	M.M[3][0] = E[12] * 100.0;
	M.M[3][1] = E[14] * 100.0;
	M.M[3][2] = E[13] * 100.0;
	M.M[0][3] = M.M[1][3] = M.M[2][3] = 0.0;
	M.M[3][3] = 1.0;
	FTransform T;
	T.SetFromMatrix(M);
	return T;
}

FTransform MatToUE(const TArray<double>& E)
{
	if (E.Num() < 16) return FTransform::Identity;
	return MatToUE(E.GetData());
}

void Compose(const FVector& P, const FQuat& Q, const FVector& S, double* Out)
{
	// three's Matrix4.compose
	const double x = Q.X, y = Q.Y, z = Q.Z, w = Q.W;
	const double x2 = x + x, y2 = y + y, z2 = z + z;
	const double xx = x * x2, xy = x * y2, xz = x * z2, yy = y * y2, yz = y * z2, zz = z * z2;
	const double wx = w * x2, wy = w * y2, wz = w * z2;
	Out[0] = (1 - (yy + zz)) * S.X; Out[1] = (xy + wz) * S.X; Out[2] = (xz - wy) * S.X; Out[3] = 0;
	Out[4] = (xy - wz) * S.Y; Out[5] = (1 - (xx + zz)) * S.Y; Out[6] = (yz + wx) * S.Y; Out[7] = 0;
	Out[8] = (xz + wy) * S.Z; Out[9] = (yz - wx) * S.Z; Out[10] = (1 - (xx + yy)) * S.Z; Out[11] = 0;
	Out[12] = P.X; Out[13] = P.Y; Out[14] = P.Z; Out[15] = 1;
}

void Decompose(const double* E, FVector& P, FQuat& Q, FVector& S)
{
	double sx = FVector(E[0], E[1], E[2]).Size(), sy = FVector(E[4], E[5], E[6]).Size(), sz = FVector(E[8], E[9], E[10]).Size();
	const double det = E[0] * (E[5] * E[10] - E[9] * E[6]) - E[4] * (E[1] * E[10] - E[9] * E[2]) + E[8] * (E[1] * E[6] - E[5] * E[2]);
	if (det < 0) sx = -sx;
	P = FVector(E[12], E[13], E[14]);
	const double m11 = E[0] / sx, m12 = E[4] / sy, m13 = E[8] / sz;
	const double m21 = E[1] / sx, m22 = E[5] / sy, m23 = E[9] / sz;
	const double m31 = E[2] / sx, m32 = E[6] / sy, m33 = E[10] / sz;
	// three's Quaternion.setFromRotationMatrix
	const double tr = m11 + m22 + m33;
	if (tr > 0) { const double s = 0.5 / FMath::Sqrt(tr + 1.0); Q = FQuat((m32 - m23) * s, (m13 - m31) * s, (m21 - m12) * s, 0.25 / s); }
	else if (m11 > m22 && m11 > m33) { const double s = 2.0 * FMath::Sqrt(1.0 + m11 - m22 - m33); Q = FQuat(0.25 * s, (m12 + m21) / s, (m13 + m31) / s, (m32 - m23) / s); }
	else if (m22 > m33) { const double s = 2.0 * FMath::Sqrt(1.0 + m22 - m11 - m33); Q = FQuat((m12 + m21) / s, 0.25 * s, (m23 + m32) / s, (m13 - m31) / s); }
	else { const double s = 2.0 * FMath::Sqrt(1.0 + m33 - m11 - m22); Q = FQuat((m13 + m31) / s, (m23 + m32) / s, 0.25 * s, (m21 - m12) / s); }
	S = FVector(sx, sy, sz);
}

FQuat Euler(double X, double Y, double Z, const TCHAR* Order)
{
	// three's Quaternion.setFromEuler
	const double c1 = FMath::Cos(X / 2), c2 = FMath::Cos(Y / 2), c3 = FMath::Cos(Z / 2);
	const double s1 = FMath::Sin(X / 2), s2 = FMath::Sin(Y / 2), s3 = FMath::Sin(Z / 2);
	const FString O(Order);
	if (O == TEXT("YXZ")) return FQuat(s1 * c2 * c3 + c1 * s2 * s3, c1 * s2 * c3 - s1 * c2 * s3, c1 * c2 * s3 - s1 * s2 * c3, c1 * c2 * c3 + s1 * s2 * s3);
	if (O == TEXT("ZXY")) return FQuat(s1 * c2 * c3 - c1 * s2 * s3, c1 * s2 * c3 + s1 * c2 * s3, c1 * c2 * s3 + s1 * s2 * c3, c1 * c2 * c3 - s1 * s2 * s3);
	if (O == TEXT("ZYX")) return FQuat(s1 * c2 * c3 - c1 * s2 * s3, c1 * s2 * c3 + s1 * c2 * s3, c1 * c2 * s3 - s1 * s2 * c3, c1 * c2 * c3 + s1 * s2 * s3);
	if (O == TEXT("YZX")) return FQuat(s1 * c2 * c3 + c1 * s2 * s3, c1 * s2 * c3 + s1 * c2 * s3, c1 * c2 * s3 - s1 * s2 * c3, c1 * c2 * c3 - s1 * s2 * s3);
	if (O == TEXT("XZY")) return FQuat(s1 * c2 * c3 - c1 * s2 * s3, c1 * s2 * c3 - s1 * c2 * s3, c1 * c2 * s3 + s1 * s2 * c3, c1 * c2 * c3 + s1 * s2 * s3);
	return FQuat(s1 * c2 * c3 + c1 * s2 * s3, c1 * s2 * c3 - s1 * c2 * s3, c1 * c2 * s3 + s1 * s2 * c3, c1 * c2 * c3 - s1 * s2 * s3); // XYZ
}

void Multiply(const double* A, const double* B, double* Out)
{
	double R[16];
	for (int32 C = 0; C < 4; ++C)
	{
		for (int32 Rw = 0; Rw < 4; ++Rw)
		{
			double V = 0;
			for (int32 K = 0; K < 4; ++K) V += A[K * 4 + Rw] * B[C * 4 + K];
			R[C * 4 + Rw] = V;
		}
	}
	FMemory::Memcpy(Out, R, sizeof(R));
}

FRotator CameraRotation(double Yaw, double Pitch, double Roll)
{
	const FQuat Q = Euler(Pitch, Yaw, Roll, TEXT("YXZ"));
	double M[16];
	Compose(FVector::ZeroVector, Q, FVector::OneVector, M);
	// forward is -Z, up is +Y, in three's space
	const FVector F(-M[8], -M[9], -M[10]), U(M[4], M[5], M[6]);
	return FRotationMatrix::MakeFromXZ(DirToUE(F), DirToUE(U)).Rotator();
}

FLinearColor Hex(uint32 Rgb)
{
	return FLinearColor(FColor((Rgb >> 16) & 255, (Rgb >> 8) & 255, Rgb & 255));
}
}

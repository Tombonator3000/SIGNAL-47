#pragma once

#include "CoreMinimal.h"

// The web version's space and Unreal's. The game logic keeps the web version's terms
// (three.js: metres, Y up, the camera looking down -Z), so story code ports line for line;
// only drawing converts. Unreal's glTF import maps a point (x, y, z) to (100x, 100z, 100y)
// (measured with a test triangle, 21 October 2026), which swaps two axes; transforms are
// therefore conjugated with that swap, which keeps rotations proper.
namespace S47Three
{
	inline FVector ToUE(double X, double Y, double Z) { return FVector(X * 100.0, Z * 100.0, Y * 100.0); }
	inline FVector ToUE(const FVector& T) { return ToUE(T.X, T.Y, T.Z); }
	inline FVector DirToUE(const FVector& T) { return FVector(T.X, T.Z, T.Y); }
	inline FVector ToThree(const FVector& U) { return FVector(U.X / 100.0, U.Z / 100.0, U.Y / 100.0); }

	// A three.js matrix (16 numbers, column-major, metres) as an Unreal transform (cm).
	SIGNAL47_API FTransform MatToUE(const double* E);
	SIGNAL47_API FTransform MatToUE(const TArray<double>& E);
	// A three.js matrix from position, quaternion and scale (three's compose).
	SIGNAL47_API void Compose(const FVector& P, const FQuat& Q, const FVector& S, double* Out);
	// Decompose a three.js matrix into position, quaternion (three's x, y, z, w) and scale.
	SIGNAL47_API void Decompose(const double* E, FVector& P, FQuat& Q, FVector& S);
	// A quaternion from three's Euler angles in the given order ("XYZ" by default).
	SIGNAL47_API FQuat Euler(double X, double Y, double Z, const TCHAR* Order = TEXT("XYZ"));
	SIGNAL47_API void Multiply(const double* A, const double* B, double* Out); // Out = A * B (column-major)

	// The camera's rotation from three's player yaw and pitch (Euler YXZ, looking down -Z).
	SIGNAL47_API FRotator CameraRotation(double Yaw, double Pitch, double Roll = 0.0);
	// three's vertical field of view as Unreal's horizontal one, for an aspect ratio.
	inline float HorizontalFov(float VerticalDeg, float Aspect)
	{
		return FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(VerticalDeg) * 0.5f) * Aspect));
	}
	// three's Color.set(hex): sRGB to linear.
	SIGNAL47_API FLinearColor Hex(uint32 Rgb);
}

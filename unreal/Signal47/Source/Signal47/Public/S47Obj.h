#pragma once

#include "CoreMinimal.h"

class AS47Web;
class UTexture;
class ULightComponent;

// The web version's scene as its TypeScript code holds it. The port names the same things by
// the same paths ("room.objs.mug", "world.site.objs.drawer", "yard.dryPrints[2]"; every path
// the exporter found, Content/WebData/manifest.json "handles") and reads and writes them in
// three.js terms: metres, y up, Euler angles in radians in the XYZ order, linear colours.
//
//   FS47Obj Mug = Web->Obj(TEXT("room.objs.mug"));
//   Mug.SetVisible(false);                     // mug.visible = false
//   Mug.SetPosition(FVector(1, 0.8, -2));      // mug.position.set(1, 0.8, -2)
//   Mug.SetRotZ(0.6);                          // mug.rotation.z = 0.6
//   Mug.SetColor(S47Three::Hex(0x0d1410));     // (mug.material as MeshStandardMaterial).color.set(0x0d1410)
//
// A handle that names nothing is harmless: every call on it does nothing and reads give zero.
struct SIGNAL47_API FS47Obj
{
	AS47Web* W = nullptr;
	int32 I = -1;

	FS47Obj() = default;
	FS47Obj(AS47Web* InW, int32 InI) : W(InW), I(InI) {}
	bool IsValid() const;
	explicit operator bool() const { return IsValid(); }
	bool operator==(const FS47Obj& O) const { return W == O.W && I == O.I; }
	FString Name() const;

	// the local transform (position, rotation, quaternion, scale), as three's Object3D
	FVector Position() const;
	void SetPosition(const FVector& P);
	void SetX(double V);
	void SetY(double V);
	void SetZ(double V);
	FVector Rotation() const; // Euler XYZ
	void SetRotation(const FVector& R);
	void SetRotX(double V);
	void SetRotY(double V);
	void SetRotZ(double V);
	FQuat Quaternion() const;
	void SetQuaternion(const FQuat& Q);
	FVector Scale() const;
	void SetScale(const FVector& S);
	void SetScale(double S) { SetScale(FVector(S)); }

	bool Visible() const;           // its own flag (object.visible)
	bool Shown() const;             // it and every parent visible
	void SetVisible(bool bOn);

	// world space (three.js world, metres)
	FVector WorldPosition() const;  // getWorldPosition
	FQuat WorldQuaternion() const;  // getWorldQuaternion
	FVector WorldDirection() const; // getWorldDirection: its +Z in the world
	FVector LocalToWorld(const FVector& P) const;
	FVector WorldToLocal(const FVector& P) const;
	FBox WorldBox() const;          // Box3().setFromObject: every mesh under it
	void LookAt(const FVector& Target); // Object3D.lookAt (its +Z towards the point)

	FS47Obj Parent() const;
	TArray<FS47Obj> Children() const;
	FS47Obj Find(const FString& Name) const; // getObjectByName, in it and under it
	void Traverse(TFunctionRef<void(FS47Obj)> Fn) const;

	// The look of every mesh in it and under it. Each gets its own copy of its materials the first
	// time; colours are linear (three's Color), S47Three::Hex for a hex value.
	void SetColor(const FLinearColor& C) const;
	void SetEmissive(const FLinearColor& C, float Intensity = 1.f) const;
	void SetOpacity(float A) const;
	// A picture on it (material.map), for instance a photo. bFlipY (the default) for a texture whose
	// row 0 is its top (a render target drawn here, a picture decoded with S47Image), as the mesh's
	// UVs expect three's flipped upload; false for a picture already stored the other way up.
	void SetMap(UTexture* T, bool bFlipY = true) const;

	// An InstancedMesh's instance K, as instancedMesh.setMatrixAt(K, compose(P, Q, S)) in its own
	// terms (the truck's wheels and needles). bFlush: mark it for drawing now (the last of a batch).
	void SetInstance(int32 K, const FVector& P, const FQuat& Q, const FVector& S, bool bFlush = true) const;
};

// A light (PointLight, SpotLight, DirectionalLight) by its path or the exported uuid.
struct SIGNAL47_API FS47Light
{
	AS47Web* W = nullptr;
	FString Uuid;

	bool IsValid() const;
	explicit operator bool() const { return IsValid(); }
	ULightComponent* Component() const;
	float Intensity() const;           // three's intensity (candela for point and spot lights)
	void SetIntensity(float V) const;
	FLinearColor Color() const;
	void SetColor(const FLinearColor& C) const;
	bool Visible() const;
	void SetVisible(bool bOn) const;
	FVector Position() const;          // world, three.js terms
	void SetPosition(const FVector& P) const;
	float Distance() const;            // three's distance (0: no limit)
	void SetDistance(float D) const;
};

// A material shared by many meshes (kit.ts materials, a CRT screen): a change shows on every
// mesh that uses it, as in three.
struct SIGNAL47_API FS47Mat
{
	AS47Web* W = nullptr;
	int32 Id = -1;

	bool IsValid() const { return W && Id >= 0; }
	explicit operator bool() const { return IsValid(); }
	void SetColor(const FLinearColor& C) const;
	void SetEmissive(const FLinearColor& C, float Intensity = 1.f) const;
	void SetOpacity(float A) const;
	void SetScalar(const FName& Param, float V) const;
	void SetMap(UTexture* T, bool bFlipY = true) const;
};

// A place to stand in the web version's terms (an area's anchors: {x, z, yaw})
struct SIGNAL47_API FS47Pose
{
	double X = 0, Z = 0, Yaw = 0;
	bool bValid = false;
};

// A rectangle on the ground in the web version's terms (a walk zone, a room's bounds).
struct SIGNAL47_API FS47Area
{
	double MinX = 0, MaxX = 0, MinZ = 0, MaxZ = 0;
	bool bValid = false;
	bool Contains(double X, double Z, double Margin = 0) const
	{
		return bValid && X >= MinX - Margin && X <= MaxX + Margin && Z >= MinZ - Margin && Z <= MaxZ + Margin;
	}
	bool Contains(const FVector& P, double Margin = 0) const { return Contains(P.X, P.Z, Margin); }
};

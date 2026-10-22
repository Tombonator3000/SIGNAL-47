#pragma once

#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"

class AS47Web;
class USceneComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMaterialBillboardComponent;
class UTexture;
struct FS47Cam;

// What the web version builds in code at run time and hangs in its scene (the one standing where the
// player was taken and the one lying in the grass, story/abduction.ts; the walker, story/recording.ts;
// the outlines over you on waking, core/roadside.ts Silhouette; the third one, story/theThird.ts): none
// of it is in the export, so it is built here the same way, out of the engine's basic shapes, one
// scene component per three.js Object3D, in the web version's terms (metres, y up; position, an Euler
// rotation in its order, scale). The geometries map onto the engine's shapes:
//
//   BoxGeometry(w, h, d)        the 1 m cube, scaled (w, h, d)
//   SphereGeometry(r)           the 1 m sphere, scaled 2r (its segments are the engine's)
//   CapsuleGeometry(r, length)  a cylinder `length` long and a sphere at each end
//   PlaneGeometry(w, h)         the 1 m plane, stood up to face +Z
//   Sprite                      a material billboard (the glints: M_WebGlow)
//
// MeshBasicMaterial is one of the web world's unlit masters (M_Web_Unlit_*) with its colour, map and
// opacity; a material shared by several meshes in three is one dynamic instance shared here too.
// renderOrder is the translucent sort priority. Nothing here collides or casts a shadow.
struct SIGNAL47_API FS47Node
{
	TWeakObjectPtr<USceneComponent> C;
	FVector Position = FVector::ZeroVector;
	FVector Rotation = FVector::ZeroVector; // Euler, radians
	FVector Scale = FVector::OneVector;
	const TCHAR* Order = TEXT("XYZ");
	/** object.visible (drawn only with every parent visible, as three draws it), and its parent. */
	bool bVisible = true;
	FS47Node* Parent = nullptr;
	/** The transform as set above, onto the component (object.position.set ...; the matrix updated). */
	void Apply() const;
	/** localToWorld / worldToLocal, three.js world terms. */
	FVector LocalToWorld(const FVector& L) const;
	FVector WorldToLocal(const FVector& W) const;
	FVector WorldPosition() const { return LocalToWorld(FVector::ZeroVector); }
	/** getWorldQuaternion, three's terms. */
	FQuat WorldQuaternion() const;
};

class SIGNAL47_API FS47Group
{
public:
	/** Under the web world's root (scene.add), or under one of its nodes' components (parent.add). */
	FS47Group(AS47Web* InWeb, const FString& InName, USceneComponent* Parent = nullptr);
	~FS47Group();
	FS47Group(const FS47Group&) = delete;
	FS47Group& operator=(const FS47Group&) = delete;

	FString Name;
	/** The group itself (THREE.Group): its position, rotation and scale. */
	FS47Node& Root() { return *Nodes[0]; }
	const FS47Node& Root() const { return *Nodes[0]; }
	/** object.visible, and whether it hangs in the scene (scene.add / removeFromParent): drawn only with both. */
	void SetVisible(bool bOn);
	bool Visible() const { return bVisible; }
	void AddToScene();
	void RemoveFromScene();
	bool InScene() const { return bInScene; }
	/** A part's own object.visible changed: everything drawn again as it stands. */
	void Refresh();

	// ---------- the parts ----------
	UMaterialInstanceDynamic* Basic(const FLinearColor& Color, const TCHAR* Base = TEXT("M_Web_Unlit_Opaque"));
	FS47Node* Group(FS47Node* Parent, const FVector& P = FVector::ZeroVector, const FVector& R = FVector::ZeroVector, const FVector& S = FVector::OneVector, const TCHAR* Order = TEXT("XYZ"));
	FS47Node* Box(FS47Node* Parent, double Wd, double Ht, double Dp, UMaterialInterface* M, const FVector& P, const FVector& R = FVector::ZeroVector, const FVector& S = FVector::OneVector);
	FS47Node* Sphere(FS47Node* Parent, double Rad, UMaterialInterface* M, const FVector& P, const FVector& S = FVector::OneVector);
	/** CapsuleGeometry(r, length) about its local y, as a group (rotate and scale it as the mesh). */
	FS47Node* Capsule(FS47Node* Parent, double Rad, double Len, UMaterialInterface* M, const FVector& P, const FVector& R = FVector::ZeroVector, const FVector& S = FVector::OneVector);
	/** PlaneGeometry(w, h) facing +Z, its middle at P. */
	FS47Node* Plane(FS47Node* Parent, double Wd, double Ht, UMaterialInterface* M, const FVector& P, const FVector& R = FVector::ZeroVector);
	/** A sprite (additive glow, M_WebGlow), w by h metres. */
	FS47Node* Sprite(FS47Node* Parent, UMaterialInstanceDynamic* M, const FVector& P);
	void SpriteSize(FS47Node* B, double Wd, double Ht);
	/** renderOrder for everything under it (translucent sort priority). */
	void RenderOrder(int32 Order);

private:
	TWeakObjectPtr<AS47Web> Web;
	TArray<TUniquePtr<FS47Node>> Nodes;
	TArray<TStrongObjectPtr<UMaterialInstanceDynamic>> Mids;
	bool bVisible = true, bInScene = false;
	FS47Node* Mesh(FS47Node* Parent, const TCHAR* Shape, UMaterialInterface* M, const FVector& P, const FVector& R, const FVector& S, const TCHAR* Order = TEXT("XYZ"));
};

namespace S47Figures
{
	/** A web world master by name (/Game/Web/Base/<Name>), or null. */
	SIGNAL47_API UMaterialInterface* Base(const TCHAR* Name);
	/** A dynamic instance of a master with three's colour (linear) and its map. */
	SIGNAL47_API void SetColor(UMaterialInstanceDynamic* M, const FLinearColor& C);
	SIGNAL47_API void SetMap(UMaterialInstanceDynamic* M, UTexture* T);
	SIGNAL47_API void SetOpacity(UMaterialInstanceDynamic* M, double A);
	/** drive/truckTextures.ts frostGlassTex: the frost on the windshield (512 x 256, its alpha the rime,
	 *  heaviest at the edges, the driver's side a little clearer; the fern of the crystals grown in from
	 *  the rubber). Made once. */
	SIGNAL47_API class UTexture2D* FrostGlass();
	SIGNAL47_API TArray<FColor> FrostGlassPixels();
}

// story/recording.ts Walker: someone in a SARO jacket with the reflective band across it, jointed at
// the hips and shoulders so it can walk (the same boxes and colours as the one standing has always
// had). Built facing -z, the way the player faces at yaw 0, from the feet. (Ported here with the time
// away, which makes the one standing where the player was taken; the recording's walk, story/
// recording.ts RecordingWalk, poses it.)
class SIGNAL47_API FS47JacketWalker
{
public:
	FS47JacketWalker(AS47Web* Web, const FString& Name);
	TUniquePtr<FS47Group> G;
	/** How much light from near the eye is on the band now (0 to 1). */
	double Shone() const { return Lit; }
	/** The reflective band in a light from near the eye (the flashlight, the headlights): it gives the
	 *  light back the way it came, as the tape does, 0 to 1; the eye the light is beside. */
	void Shine(double K, const FS47Cam* Cam = nullptr);
	/** Standing still, as it has always stood. */
	void Stand() { Pose(0, 0, 0, 0, 0); }
	/** How it is carried: the stride (-1..1 through a step), how wide (0 standing, 1 a full run's),
	 *  how far it leans forward and to the side (radians), and how high the body is lifted (metres). */
	void Pose(double Swing, double Wide, double Lean, double Roll, double Lift);

private:
	FS47Node* Body = nullptr;
	FS47Node* Legs[2] = { nullptr, nullptr };
	FS47Node* Arms[2] = { nullptr, nullptr };
	FS47Node* Head = nullptr;
	TArray<FS47Node*> Tapes; // the chest's, then the sleeves'
	UMaterialInstanceDynamic* Band = nullptr;
	UMaterialInstanceDynamic* Coat = nullptr;
	UMaterialInstanceDynamic* Dark = nullptr;
	UMaterialInstanceDynamic* GlintMat = nullptr;
	TArray<FS47Node*> Glints;
	double Lit = 0;
};

// core/roadside.ts Silhouette, as round 14's outlines have it (the painted card): about 1.2 m, a flat
// picture that turns round its own upright to whoever looks, dark, the light only along its edge,
// never a face; 'bent' is only ever near, over you on waking: the standing picture with its head
// brought down to you and a little to one side (makeBent). The figure in code the web version keeps
// until the pictures come is not needed: the pictures are shipped with the build (painted is false
// only when they are missing, and then nothing stands over you, as in the web version).
class SIGNAL47_API FS47CardSilhouette
{
public:
	enum class EPose : uint8 { Stand, Turned, Far, Bent };
	FS47CardSilhouette(AS47Web* Web, const FString& Name, uint32 Edge, EPose Want = EPose::Stand);
	TUniquePtr<FS47Group> G;
	/** The night may show it now (the pictures are here: nothing to ask for). */
	void Wake() {}
	void Pose(EPose P);
	/** How high the middle of its head is, in metres, as it stands now. */
	double HeadY() const;
	/** The colour of the light along its edge (what is behind it where it stands). */
	void Rim(uint32 Hex);
	bool Painted() const { return bPainted; }
	/** card.onBeforeRender: turned round its upright to the eye (three's world terms). */
	void Face(const FVector& Eye);

private:
	FS47Node* Card = nullptr;
	UMaterialInstanceDynamic* Mat = nullptr;
	EPose Want = EPose::Stand;
	bool bPainted = false;
};

#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "S47Seen.h"
#include "S47UI.h"

class AS47Web;
class US47FieldCamera;

// web/src/core/album.ts: the free camera and the album (todo.md, after They Are Here; SPILLDESIGN.md,
// point 6). Once the field camera is had, the shutter takes a picture of anything, anywhere, away
// from the case's own subjects (Chapter1.check, `near`): the case frames keep their numbers and their
// sheet film, the free frames come out of the camera's instant back, unnumbered, 36 in a night. An
// exposure that was not the player's (story/abduction.ts) is one of the 36 too.
//
// A free frame is the scene rendered once more from the camera, 640 x 400 (320 x 200 where the saves
// keep their pictures inline), with the case prints' film look. It is kept with the case and shown in
// the notebook's album (ui/album.ts, SS47Notebook) with a caption in the player's hand: the one or two
// most prominent of the things main.ts names (story/albumSubjects.ts, S47AlbumSubjects.h) that are in
// the frame with a clear line of sight to them. Things that should not be there get a line of their
// own. And now and then, outdoors at night and more often towards morning, the film keeps someone at
// the edge of the frame whom the eye did not see, with a faint pale smear round them: "Nobody was
// there when I took it." At most twice in a night, never in the first three frames. A glimpse the
// frame catches (core/glimpses.ts) gets the same smear and the same line.
//
// Once in the night, closer than that: the first free frame taken out of doors in chapter four keeps,
// at its edge and a foot from the lens, the side of a pale head and one black eye with the flash in
// it (core/filmFace.ts, S47FilmFace.h). It is never in the scene. The caption is written once the
// print has come up; when it has, the game says so once, and the first time the developed print is
// looked at closely the night goes quiet and the notebook gets a line (main.ts). Kept with the album.
//
// The figure on the film and its two smears are the web version's own (Silhouette('album:ghost') and
// the two 'album:halo' planes under the scene), in the export as they were built, hidden.

/** Something that can be named under a photograph. */
struct FS47Subject
{
	/** As it is written under the photograph: 'the antennas', 'N. Vega'. */
	FString Name;
	/** Where it is (world terms): one point, or several for a group of things (the array). */
	TArray<FVector> At;
	/** Its size in metres (default 1), or its angular size in degrees (the sky). */
	TOptional<double> R, Ang;
	/** The farthest it is named from (default 120 m), and how much it counts (default 1). */
	TOptional<double> Max, W;
	/** Things that count as one: only the most prominent of them is named. */
	FString Group;
	/** A line of its own, always written when it is in the photograph. */
	FString Odd;
	/** Its height in metres, for a figure that must show on the print to be written about. */
	TOptional<double> H;
	/** In the sky: the line of sight is checked only so far out. */
	bool bSky = false;
	/** Kept on the print as one of its targets when it is in the photograph ('starhole'). */
	FString Target;
};

struct FS47AlbumShot
{
	FString Id, Url;
	double Clock = 0;
	FString Place, Text;
	bool bOdd = false;
	/** Where the things the story looks for are on the print (u, v from the top left), by target. */
	TArray<TPair<FString, FVector2D>> Targets;
};
/** The print with the face: which one, whether the game has said it has come up, and whether it
 *  has been looked at closely since. */
struct FS47FaceSave { FString Id; bool bUp = false, bSeen = false; };
/** What is kept with the case (GameState.album). */
struct FS47AlbumSave { TArray<FS47AlbumShot> Shots; int32 Ghosts = 0; TOptional<FS47FaceSave> Face; };

struct FS47AlbumDeps
{
	US47FieldCamera* Fcam = nullptr;
	AS47Web* Web = nullptr;
	/** What can be named in a photograph here and now. */
	TFunction<TArray<FS47Subject>()> Subjects;
	TFunction<FString()> Place;
	TFunction<double()> Clock;
	/** Exposures on the roll that were not the player's. */
	TFunction<int32()> Extra;
	/** Before the render (the web version culls the antennas for the photo camera). */
	TFunction<void()> Before;
	/** The ground's height where the film may keep someone, or unset (not here, not now). */
	TFunction<TFunction<double(double, double)>()> GhostGround;
	/** The saves have no room for big pictures: smaller ones. */
	TFunction<bool()> Small;
	/** The shutter's sound and flash, a word on the roll, the save. */
	TFunction<void(const FS47AlbumShot&, bool, int32)> Done;
	/** Attach only features actually framed at exposure time, before persistent storage. */
	TFunction<void(FS47AlbumShot&)> Tag;
	/** A free frame taken here and now may keep the face (main.ts: chapter four, out of doors, on foot). */
	TFunction<bool()> Face;
	/** The print with the face has been looked at closely for the first time, come up. */
	TFunction<void(const FS47AlbumShot&)> FaceSeen;
};

class SIGNAL47_API FS47Album
{
public:
	static constexpr int32 ROLL = 36;
	static const TCHAR* NOBODY;
	/** The caption on the print with the face: the words under the figures far off, now under a face at the lens. */
	static const TCHAR* FACE_LINE;

	explicit FS47Album(FS47AlbumDeps InD);

	FS47AlbumSave Data;
	/** Tests: the next frame keeps the figure, where there is ground for it. */
	bool bForceGhost = false;
	/** The developer menu: the next free frame keeps the face, anywhere, even if one has already. */
	bool bForceFace = false;
	/** Tests: what the last caption was made from, how bright the frame was, how long it took. */
	struct FLast { TArray<FString> Names, Odd; double Mean = 0, Ms = 0, LookMs = 0; bool bGhost = false, bFace = false; TOptional<FVector2D> Eye; };
	TOptional<FLast> Last;

	const TArray<FS47AlbumShot>& Shots() const { return Data.Shots; }
	/** Free frames left on the roll tonight. */
	int32 Left() const;
	/** Take a free frame now (Chapter1.shutter, when the case has no frame to take here). */
	FS47AlbumShot Expose();
	/** What a frame taken now would name, without taking it (the tests). */
	TPair<TArray<FString>, TArray<FString>> Peek();
	/** A save opened, or a new night. */
	void Restore(const S47J::FObj& V);
	S47J::FObj SaveData() const;
	/** The print with the face, if the roll has one. */
	const FS47AlbumShot* FaceShot() const;
	/** Every frame of play: the print with the face, the moment it has come up (once in the night). */
	const FS47AlbumShot* FaceUp(double Clock);
	/** A print opened big in the notebook: the first close look at the face, come up. */
	void Looked(const FString& Id);
	/** For the notebook: each print, how far it has come up by now. */
	FS47AlbumView View(double Clock);
	/** Nothing solid and shown between From and To, looked for only Cap metres out (the sky). */
	bool InSight(const FVector& From, const FVector& To, double Cap = 1e30) const { return !Blocked(From, To, 1, Cap); }

	static FString RollText(int32 Left);
	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47AlbumDeps D;
	FS47Obj Ghost;
	TArray<FS47Obj> Halos;
	struct FSeen { const FS47Subject* S = nullptr; FVector P = FVector::ZeroVector; double Score = 0; };

	FS47Cam PhotoCam() const;
	void PlaceHalos(const TArray<FVector>& Figs);
	void ShowFilmOnly(bool bOn);
	double Dev(const FS47AlbumShot& S, double Clock) const;
	TArray<FSeen> InFrame(const TArray<FS47Subject>& List) const;
	bool Blocked(const FVector& From, const FVector& To, double R, double Cap = 1e30) const;
	TOptional<FVector> PlaceGhost(const TFunction<double(double, double)>& Ground);
};

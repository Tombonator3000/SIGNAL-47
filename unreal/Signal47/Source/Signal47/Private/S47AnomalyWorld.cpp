#include "S47AnomalyWorld.h"
#include "S47Album.h"
#include "S47Anomaly.h"
#include "S47Audio.h"
#include "S47FieldCamera.h"
#include "S47Image.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47RoadTerrain.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	double DistanceXZ(const FVector& A, const FVector& B) { return FVector2D(A.X - B.X, A.Z - B.Z).Size(); }
	FS47Obj Part(AS47Web* W, const FS47Obj& Parent, const FString& Name, const FVector& At, const FVector& Size, uint32 Color, bool bSphere = false, float Glow = 0)
	{
		const FS47WebNode* N = W ? W->Node(Parent.I) : nullptr;
		if (!N || !N->Comp.IsValid()) return {};
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, bSphere ? TEXT("/Engine/BasicShapes/Sphere.Sphere") : TEXT("/Engine/BasicShapes/Cube.Cube"));
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
		if (!Mesh || !Base) return {};
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(W, *Name);
		C->SetStaticMesh(Mesh); C->SetMobility(EComponentMobility::Movable);
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		C->SetCastShadow(false); C->SetupAttachment(N->Comp.Get());
		C->SetRelativeLocation(S47Three::ToUE(At)); C->SetRelativeScale3D(S47Three::DirToUE(Size));
		UMaterialInstanceDynamic* Mat = UMaterialInstanceDynamic::Create(Base, W);
		Mat->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Color));
		Mat->SetVectorParameterValue(TEXT("Emissive"), S47Three::Hex(Color));
		Mat->SetScalarParameterValue(TEXT("EmissivePower"), Glow);
		C->SetMaterial(0, Mat); C->RegisterComponent();
		return W->ObjAt(W->AdoptNode(C, Parent.I, Name));
	}
}

FVector FS47AnomalyWorld::World(const FVector& P) const { return Town ? Town->Group.LocalToWorld(P) : P; }
FVector FS47AnomalyWorld::ReferenceAt() const { return World(Town ? Town->PilotCutoutAt(false) + FVector(0, .62, 0) : FVector(20.31, .62, 2062.3)); }
FVector FS47AnomalyWorld::ChangedAt() const { return World(Town ? Town->PilotCutoutAt(true) + FVector(0, .62, 0) : FVector(17.8, .62, 2044)); }
FVector FS47AnomalyWorld::LampAt() const { return Lamp ? Lamp.WorldPosition() : World(FVector(-14.1, 1.65, 2130.4)); }
FVector FS47AnomalyWorld::StudioAt() const
{
	const FS47Obj Studio = Town ? Town->Proxy(TEXT("studio")) : FS47Obj();
	return Studio ? Studio.WorldPosition() : World(FVector(19.7, 1.4, 2036));
}
bool FS47AnomalyWorld::Here() const { return M && M->World && (M->World->Area == TEXT("diner") || M->World->Area == TEXT("road")) && !M->World->bDriving; }
bool FS47AnomalyWorld::Free() const { return M && M->UI && !M->UI->Modal() && !M->OneAtATime() && !(M->Audio && M->Audio->PhonePlaying()) && M->Story && !M->Story->IsCinematic(); }
void FS47AnomalyWorld::Toast(const FString& Text) const { if (M && M->UI) M->UI->Toast(Text, 5.5); }

void FS47AnomalyWorld::Begin()
{
	// Same-candidate graphics control only; normal play always builds the pilot.
	bDisabled = FParse::Param(FCommandLine::Get(), TEXT("S47AnomalyWorldOff"));
	if (bDisabled) return;
	if (!M || !M->TownLayer || !M->TownLayer->Town || !M->Anomaly || !M->Web || !M->Inter) return;
	Town = M->TownLayer->Town.Get();
	// Loading another case must not retain this case's short source or timer.
	if (M->Audio) M->Audio->Stop(TEXT("anomaly-printer"), .04f);
	SoundLeft = SoundRetry = 0;
	if (!bBuilt) { InstallGeometry(); bBuilt = true; }
	PlaceFigure();
}

void FS47AnomalyWorld::InstallGeometry()
{
	AS47Web* W = M->Web;
	// The old window was a baked picture of a second alien. Replace only its display
	// surface with an empty backing; one exported cardboard object now occupies both places.
	TArray<FColor> Back; Back.Init(FColor(39, 29, 21), 4);
	for (const FString& P : W->PathsUnder(TEXT("world.town.runtime.panels.display."))) W->Obj(P).SetMap(S47Image::FromPixels(Back, 2, 2));
	const FS47Obj Figure = Town->CutoutObject();
	Part(W, Figure, TEXT("AnomalyPriceTag"), FVector(-.16, .37, -.028), FVector(.22, .13, .018), 0xa57b22, false, 0);
	Part(W, Figure, TEXT("AnomalySplitEdge"), FVector(.115, .17, -.029), FVector(.012, .15, .012), 0x24190d);
	Part(W, Figure, TEXT("AnomalySplitEdgeNick"), FVector(.12, .235, -.03), FVector(.04, .018, .014), 0xa88b60);
	if (const FS47WebNode* N = W->Node(Figure.I); N && N->Comp.IsValid())
	{
		UTextRenderComponent* Price = NewObject<UTextRenderComponent>(W, TEXT("AnomalyPriceInk"));
		Price->SetupAttachment(N->Comp.Get()); Price->SetMobility(EComponentMobility::Movable);
		Price->SetRelativeLocation(S47Three::ToUE(FVector(-.16, .35, -.041))); Price->SetRelativeRotation(FRotator(0, -90, 0));
		Price->SetHorizontalAlignment(EHTA_Center); Price->SetWorldSize(6.f);
		Price->SetTextRenderColor(FColor::Black); Price->SetText(FText::FromString(TEXT("$3.50"))); Price->RegisterComponent();
	}
	// A weak opal work lamp over a narrow pale board: head and shoulders from the
	// pavement, ordinary lamp/bracket/board when the player steps sideways. No live figure.
	// Keep the ordinary lamp separate from the existing woman on the bench.
	const double Ground = Town->FloorAt(World(FVector(-14.1, 0, 2130.4)).X, World(FVector(-14.1, 0, 2130.4)).Z) - Town->Group.WorldPosition().Y;
	Lamp = Part(W, Town->Group, TEXT("AnomalyOpalLamp"), FVector(-14.1, Ground + 1.65, 2130.4), FVector(.27, .3, .27), 0xe1cf9b, true, .45f);
	LampShade = Part(W, Town->Group, TEXT("AnomalyPaleBoard"), FVector(-14.8, Ground + .94, 2130.4), FVector(.10, 1.05, .53), 0xaaa080, false, .08f);
	Part(W, Town->Group, TEXT("AnomalyLampBracket"), FVector(-14.5, Ground + 1.65, 2130.4), FVector(.78, .045, .045), 0x393832);
	Part(W, Town->Group, TEXT("AnomalyLampWire"), FVector(-14.85, Ground + 1.92, 2130.4), FVector(.025, .6, .025), 0x292a25);
	Part(W, Town->Group, TEXT("AnomalyReferenceMark"), FVector(17.6, S47RoadTerrain::SurfaceY(17.6, 2062.3) + .025, 2062.3), FVector(.45, .025, .65), 0xb8a67b);
	M->Inter->Add({ TEXT("anomaly:figure"), Figure, [this] { return M->Anomaly->Data.A02Moved ? TEXT("Inspect the cardboard figure") : TEXT("Record the souvenir display"); }, [this] { InspectFigure(); }, 4.5,
		[this] { return FString(TEXT("A cardboard souvenir. Yellow $3.50 tag on its lower right; a split in the lower left edge. The same marks can be checked on a photograph.")); }, 9 });
	M->Inter->Add({ TEXT("anomaly:lamp"), Lamp, [] { return FString(TEXT("Check the pale shape from the side")); }, [this] { InspectLamp(); }, 8,
		[] { return FString(TEXT("A weak light over a pale upright shape. A side view should separate the parts. Take a photograph first, then step along the pavement and check it.")); }, 14 });
	// Extend the existing window action without stealing its proxy before the sound.
	// Earl's buzzer remains the separate, unchanged amDoor interaction.
	if (FS47Interactable* I = M->Inter->Get(TEXT("town:studio")))
	{
		const auto OriginalUse = I->Use; const auto OriginalLabel = I->Label;
		I->Use = [this, OriginalUse] { if (M->Anomaly->Data.P01Heard) InspectStudio(); else if (OriginalUse) OriginalUse(); };
		I->Label = [this, OriginalLabel] { return Here() && M->Anomaly->Data.P01Heard ? FString(TEXT("Check the closed studio facade")) : OriginalLabel ? OriginalLabel() : FString(); };
	}
	if (FS47Interactable* I = M->Inter->Get(TEXT("town:souvenir"))) I->Use = [this] { InspectFigure(); };
	// Named alias for the same follow-up in deterministic input scenarios; no extra hit surface.
	M->Inter->Add({ TEXT("anomaly:studio"), {}, {}, [this] { InspectStudio(); }, 5.5 });
}

void FS47AnomalyWorld::PlaceFigure()
{
	if (!Town || !M || !M->Anomaly) return;
	// A witnessed legacy shoulder event stays historical. It is not turned into a
	// completed investigation or replayed beside the studio to fill the new pilot.
	if (M->Anomaly->Data.bLegacyCutout) Town->ReleasePilotCutout();
	else Town->SetPilotCutout(M->Anomaly->Data.A02Moved);
}
void FS47AnomalyWorld::Reset()
{
	if (M && M->Audio) M->Audio->Stop(TEXT("anomaly-printer"), .04f);
	SoundLeft = SoundRetry = 0; PlaceFigure();
	if (Town && M && M->Story) Town->bAnomalyBaseline = S47Timeline::Night(M->Story->Clock()) < 0;
}
bool FS47AnomalyWorld::MayMove(bool bReference, bool bMoved, double Clock, double Distance)
{
	return bReference && !bMoved && S47Timeline::Night(Clock) >= 0 && Distance > 45;
}
bool FS47AnomalyWorld::MayPlay(bool bPending, bool bHeard, bool bPanel, bool bFree, bool bHere, double Clock)
{
	const double Day = S47Timeline::Day(Clock);
	return bPending && !bHeard && !bPanel && bFree && bHere && Day >= 4 * 3600 && Day < 12 * 3600;
}
void FS47AnomalyWorld::Step(double Dt)
{
	if (bDisabled) return;
	if (!Town) Begin();
	if (!Town || !M->Anomaly || !M->Story || !M->Player) return;
	auto& D = M->Anomaly->Data; const double Clock = M->Story->Clock();
	SoundRetry = FMath::Max(0., SoundRetry - Dt);
	// The audio source keeps real time while a notebook panel is open.
	SoundLeft = FMath::Max(0., SoundLeft - Dt);
	Town->bAnomalyBaseline = S47Timeline::Night(Clock) < 0 || SoundLeft > 0;
	if (!D.bLegacyCutout && Free() && MayMove(!D.ReferencePhoto.IsEmpty() || D.bDatedReference, D.A02Moved, Clock, Here() ? DistanceXZ(M->Player->Pos, ReferenceAt()) : 10000))
	{
		D.A02Moved = true; PlaceFigure(); M->Anomaly->Changed();
	}
	if (SoundLeft <= 0 && M->Audio) M->Audio->Stop(TEXT("anomaly-printer"), .03f);
	if (!Here() || !Free()) return;
	const FS47Cam Cam = FS47Cam::Of(M->Player, 5000);
	auto Looking = [&](const FVector& At, double Range) { return FVector::Distance(Cam.Pos, At) < Range && FVector::DotProduct(Cam.Forward, (At - Cam.Pos).GetSafeNormal()) > .94; };
	const FVector Standing = Town->Group.WorldToLocal(M->Player->Pos);
	if (!D.N02Observed && Standing.X > -15 && Standing.Z > 2130 && Standing.Z < 2135.5 && Looking(LampAt(), 15))
	{
		D.N02Observed = true; D.N02Clock = Clock; M->Anomaly->Changed();
		Toast(TEXT("For a moment, a person by the laundromat. Photograph the shape, then change your standing position before deciding."));
	}
	if (!D.bLegacyCutout && D.A02Moved && !D.A02Observed && Looking(ChangedAt(), 18))
	{
		D.A02Observed = true; M->Anomaly->Changed();
		Toast(TEXT("The cardboard figure is outside the display. Photograph it, then compare the yellow price tag and the split edge in the notebook."));
	}
	if (SoundRetry <= 0 && MayPlay(D.P01Pending, D.P01Heard, M->UI && M->UI->Modal(), Free(), DistanceXZ(Cam.Pos, StudioAt()) < 50, Clock))
	{
		if (!M->Audio) return;
		FS47Sound O; O.At = StudioAt(); O.Gain = .58f; O.Dur = 4.6f; O.Lp = 2400;
		if (!M->Audio->LoopSfx(TEXT("anomaly-printer"), TEXT("printer"), O)) { SoundRetry = 8; Toast(TEXT("The studio sound could not play. This observation remains pending.")); return; }
		SoundLeft = 4.6; D.P01Heard = true; D.P01Pending = false; D.P01Clock = Clock; M->Anomaly->Changed();
		Toast(TEXT("[From the closed 1420 AM studio: a printer feeds, strikes twice, feeds again, then stops.] The SARO rhythm. Check the facade."));
	}
}

bool FS47AnomalyWorld::ControlStand() const
{
	if (!M || !M->Player || !Town) return false;
	const FVector P = Town->Group.WorldToLocal(M->Player->Pos);
	return P.X > -16 && P.X < -8 && P.Z > 2136 && P.Z < 2142;
}
void FS47AnomalyWorld::InspectLamp()
{
	if (!M || !M->Anomaly || !Here()) return;
	auto& D = M->Anomaly->Data;
	if (D.N02Photo.IsEmpty()) { Toast(TEXT("Record the head-on shape with the camera before checking from another position.")); return; }
	if (!ControlStand()) { Toast(TEXT("Move a few steps along the laundromat pavement toward the next window. Look back at the lamp from its side.")); return; }
	D.N02Control = true; M->Anomaly->Changed();
	Toast(TEXT("An opal lamp on a bracket, in front of a pale board. From here the two parts separate. Take a control photograph; compare both in the notebook."));
}
void FS47AnomalyWorld::InspectFigure()
{
	if (!M || !M->Anomaly || !Here() || !M->Player) return;
	auto& D = M->Anomaly->Data;
	if (DistanceXZ(M->Player->Pos, D.A02Moved ? ChangedAt() : ReferenceAt()) > 6) return;
	if (!D.bLegacyCutout && D.A02Moved && D.A02Compared)
	{
		D.A02Followed = true; M->Anomaly->Changed();
		Toast(TEXT("The $3.50 tag and the split lower-left edge are real, not a second image in the glass. Same cardboard figure, another place. No sign of who moved it."));
	}
	else M->Anomaly->Open(D.A02Moved ? TEXT("a02") : TEXT("reference"));
}
void FS47AnomalyWorld::InspectStudio()
{
	if (!M || !M->Anomaly || !M->Anomaly->Data.P01Heard || !Here() || !M->Player || DistanceXZ(M->Player->Pos, StudioAt()) > 7) return;
	M->Anomaly->Data.P01Followed = true; M->Anomaly->Changed();
	Toast(TEXT("The door is shut. ON AIR is dark; the room behind the glass is empty. No printer is visible. The sound's source is still unresolved."));
}
void FS47AnomalyWorld::TagPhoto(FS47AlbumShot& Shot) const
{
	if (bDisabled || !Town || !Here() || !M->FieldCamera || !M->Anomaly || !M->Player || Shot.Url.IsEmpty()) return;
	const auto& D = M->Anomaly->Data;
	FString Route;
	const bool bDiag = FParse::Value(FCommandLine::Get(), TEXT("S47AnomalyRoute="), Route);
	if (bDiag) UE_LOG(LogTemp, Display, TEXT("S47: anomaly target context shot=%s eye=%s local=%s lamp=%s studio=%s control=%d"), *Shot.Id,
		*FS47Cam::Of(M->Player, 5000).Pos.ToString(), *Town->Group.WorldToLocal(M->Player->Pos).ToString(), *LampAt().ToString(), *StudioAt().ToString(), D.N02Control ? 1 : 0);
	auto Tag = [&](const FString& Id, const FVector& P, double Range)
	{
		const double Distance = DistanceXZ(M->Player->Pos, P);
		const bool bInFrame = M->FieldCamera->InFrame(P, .12, .88, .12, .88);
		if (bDiag) UE_LOG(LogTemp, Display, TEXT("S47: anomaly target %s point=%s distance=%.3f range=%.3f inFrame=%d"), *Id, *P.ToString(), Distance, Range, bInFrame ? 1 : 0);
		if (Distance > Range || !bInFrame) return;
		// A centered point behind a wall is not a documented feature. Test the actual
		// exposure line; hits on the figure itself are the intended surface.
		FCollisionQueryParams Query(SCENE_QUERY_STAT(S47AnomalyEvidence), true);
		FVector Eye = S47Three::ToUE(FS47Cam::Of(M->Player, 5000).Pos);
		const FVector End = S47Three::ToUE(P);
		const FS47Obj Target = Id.StartsWith(TEXT("anomaly:n02")) ? Lamp : Town->CutoutObject();
		const FS47WebNode* Node = M->Web->Node(Target.I);
		bool bClear = false;
		// Same rendered-surface contract as FS47Album::Blocked: invisible interaction
		// boxes and sufficiently transparent materials do not occlude an exposure.
		for (int32 Tries = 0; Tries < 24; ++Tries)
		{
			FHitResult Hit;
			if (!M->Web->GetWorld()->LineTraceSingleByChannel(Hit, Eye, End, ECC_Visibility, Query)) { bClear = true; break; }
			UPrimitiveComponent* Comp = Hit.GetComponent();
			const bool bTarget = Node && Node->Comp.IsValid() && Comp && (Comp == Node->Comp.Get() || Comp->IsAttachedTo(Node->Comp.Get()));
			bool bSolid = Comp && Comp->IsVisible();
			if (bSolid)
			{
				if (const UMaterialInterface* Mat = Comp->GetMaterial(FMath::Max<int32>(0, Hit.ElementIndex)))
				{
					const EBlendMode Blend = Mat->GetBlendMode();
					float Opacity = 1.f;
					Mat->GetScalarParameterValue(FName(TEXT("Opacity")), Opacity);
					if (Blend == BLEND_Additive || (Blend == BLEND_Translucent && Opacity < .5f)) bSolid = false;
				}
			}
			if (bDiag) UE_LOG(LogTemp, Display, TEXT("S47: anomaly target %s ray hit=%s node=%d at=%s time=%.6f targetNode=%d target=%d solid=%d"), *Id,
				*GetNameSafe(Comp), M->Web->NodeOf(Comp), *S47Three::ToThree(Hit.ImpactPoint).ToString(), Hit.Time, Target.I, bTarget ? 1 : 0, bSolid ? 1 : 0);
			if (bTarget) { bClear = true; break; }
			if (bSolid) return;
			if (Comp) Query.AddIgnoredComponent(Comp);
			Eye = Hit.ImpactPoint;
		}
		if (!bClear) return;
		const auto UV = M->FieldCamera->Project(P);
		if (bDiag) UE_LOG(LogTemp, Display, TEXT("S47: anomaly target %s clear ray front=%d uv=%.4f,%.4f"), *Id, UV.bFront ? 1 : 0, UV.U, UV.V);
		if (UV.bFront)
		{
			Shot.Targets.Add({ Id, FVector2D(UV.U, UV.V) });
			FString Caption;
			if (Id == TEXT("anomaly:reference")) Caption = TEXT("The souvenir shop display, with the cardboard figure in its window.");
			else if (Id == TEXT("anomaly:changed")) Caption = TEXT("The cardboard figure beside the radio studio.");
			else if (Id == TEXT("anomaly:n02reference")) Caption = TEXT("The bright upright outline at the laundromat.");
			else if (Id == TEXT("anomaly:n02control")) Caption = TEXT("Side view: the laundromat worklamp and upright board.");
			// Only annotate this new exposure. Retain the album's actual oddities and
			// photograph bytes; documenting the figure makes no claim about its cause.
			if (!Caption.IsEmpty() && !Shot.Text.Contains(Caption)) Shot.Text = Caption + (Shot.Text.IsEmpty() ? FString() : TEXT(" ") + Shot.Text);
		}
	};
	// The detailed tag must be legible; a distant glimpse is observation, not evidence.
	const FVector FigureAt = D.A02Moved ? ChangedAt() : ReferenceAt();
	const FVector Local = Town->Group.WorldToLocal(M->Player->Pos);
	if (!D.bLegacyCutout && Local.X < (D.A02Moved ? 17.8 : 20.0) && M->FieldCamera->InFrame(FigureAt + FVector(0, .48, 0), .08, .92, .08, .92)
		&& M->FieldCamera->InFrame(FigureAt - FVector(0, .48, 0), .08, .92, .08, .92))
		Tag(D.A02Moved ? TEXT("anomaly:changed") : TEXT("anomaly:reference"), FigureAt, 9);
	const FVector P = Town->Group.WorldToLocal(M->Player->Pos);
	if (D.N02Control && ControlStand()) Tag(TEXT("anomaly:n02control"), LampAt(), 11);
	else if (P.X > -15 && P.Z > 2130 && P.Z < 2135.5) Tag(TEXT("anomaly:n02reference"), LampAt(), 13);
}

bool FS47AnomalyWorld::SelfTest(TArray<FString>& Lines)
{
	bool Ok = !MayMove(true, false, -1, 100) && MayMove(true, false, 1, 100) && !MayMove(false, false, 1, 100) && !MayMove(true, true, 1, 100) && !MayMove(true, false, 1, 20);
	Ok &= MayPlay(true, false, false, true, true, 4 * 3600 + 10 * 60) && !MayPlay(true, false, false, true, true, 3 * 3600 + 59 * 60);
	Ok &= !MayPlay(true, false, true, true, true, 5 * 3600) && !MayPlay(true, false, false, false, true, 5 * 3600) && !MayPlay(true, true, false, true, true, 5 * 3600);
	Lines.Add(FString::Printf(TEXT("%s AnomalyWorld: persistent reference/move gates; closed-studio sound waits for active comparison, free scene and closed panel"), Ok ? TEXT("PASS") : TEXT("FAIL")));
	return Ok;
}

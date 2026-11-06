// drive/herd.ts
#include "S47Herd.h"
#include "S47FarmWorld.h"

#include "Components/MaterialBillboardComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "S47CowMeshes.h"
#include "S47Main.h"
#include "S47RoadShape.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Web.h"
#include "S47World.h"

#include <cmath>

const TCHAR* S47Herd::TAG_RANCH = TEXT("ALDERETE");
const TArray<FString>& S47Herd::Tags()
{
	static const TArray<FString> V = { TEXT("11"), TEXT("14"), TEXT("18"), TEXT("22"), TEXT("25"), TEXT("29"), TEXT("31"), TEXT("34"), TEXT("38") };
	return V;
}

namespace
{
	const double COWS[9][3] = {
		{ -27.0, 538.5, 2.3 }, { -33.5, 542.0, -0.7 }, { -26.5, 549.5, 1.1 }, { -40.5, 547.5, 2.9 }, { -30.5, 557.5, -2.2 },
		{ -46.0, 556.0, 0.6 }, { -38.0, 563.5, -1.6 }, { -52.5, 566.5, 1.9 }, { -34.0, 575.0, -2.8 },
	};
	constexpr double AZ = 26 * UE_DOUBLE_PI / 180;
	const FVector FACING(FMath::Sin(AZ), 0, -FMath::Cos(AZ));
}

FS47Herd::FS47Herd(US47Main* InM, const FString& InPath, const FString& InArea) : Area(InArea), M(InM)
{
	AS47Web* W = M ? M->Web.Get() : nullptr;
	if (W)
	{
		Group = W->Obj(InPath + TEXT(".group")); Bodies = W->Obj(InPath + TEXT(".bodies"));
		Heads = W->Obj(InPath + TEXT(".heads")); Ring = W->Obj(InPath + TEXT(".ring"));
  GroupBase=Group.Position();RingBase=Ring.Position();
		const auto* BodyNode = W->Node(Bodies.I);
		const auto* HeadNode = W->Node(Heads.I);
		auto* BodyComponent = BodyNode ? Cast<UInstancedStaticMeshComponent>(BodyNode->Comp.Get()) : nullptr;
		auto* HeadComponent = HeadNode ? Cast<UInstancedStaticMeshComponent>(HeadNode->Comp.Get()) : nullptr;
		const auto Meshes = S47CowMeshes::Load();
		if (Meshes && BodyComponent && HeadComponent && BodyComponent->GetInstanceCount() == Count() && HeadComponent->GetInstanceCount() == Count())
		{
			// Keep both original instance pools: grazing, 026, tags' look areas and missing #25
			// continue to use exactly the same transforms and save state as before.
			S47CowMeshes::Replace(BodyComponent, Meshes.Body);
			S47CowMeshes::Replace(HeadComponent, Meshes.Head);
			bNativeModels = true;
			UE_LOG(LogTemp, Display, TEXT("S47: cow models alive ready: %s, %d bodies and heads"), *Area, Count());
		}
	}
	for (int32 I = 0; I < Count(); ++I)
	{
		Ys.Add(S47RoadTerrain::SurfaceY(COWS[I][0], COWS[I][1]));
		LookAreas.Add(W ? W->Obj(FString::Printf(TEXT("%s.lookAreas[%d]"), *InPath, I)) : FS47Obj());
  LookBases.Add(LookAreas.Last().Position());
	}
	// The exported points retain their local positions. Private eye indices are not handles, so
	// match each eye to its own exported sprite rather than depending on other glow points' order.
	TArray<UMaterialBillboardComponent*> Glows;
	const FS47Obj Glow = W ? W->Obj(InPath + TEXT(".glow.points")) : FS47Obj();
	if (const FS47WebNode* N = Glow ? W->Node(Glow.I) : nullptr)
		if (USceneComponent* C = N->Comp.Get()) for (USceneComponent* Ch : C->GetAttachChildren())
			if (UMaterialBillboardComponent* B = Cast<UMaterialBillboardComponent>(Ch)) Glows.Add(B);
	bStill = true;
	for (int32 I = 0; I < Count(); ++I)
	{
		Pose(I, 0);
		for (int32 E = 0; E < 2; ++E)
		{
			const FVector P = HeadP + HeadQ.RotateVector(S47Herd::EYE[E]);
			EyeAt.Add(P);
			UMaterialBillboardComponent* Best = nullptr;
			double BestD = 0.001;
			for (UMaterialBillboardComponent* B : Glows)
			{
				const double Dist = FVector::DistSquared(S47Three::ToThree(B->GetRelativeLocation()), P);
				if (Dist < BestD) { Best = B; BestD = Dist; }
			}
			Eyes.Add(Best && Best->Elements.Num() ? Cast<UMaterialInstanceDynamic>(Best->Elements[0].Material) : nullptr);
   EyeSprites.Add(Best);
		}
	}
	bStill = false;
 ReconcileMode();
	Place(0);
}

// The exported group is road-local. Keep instance poses local and move this
// root exactly once per mode edge; all child heads/look areas/ring follow it.
bool FS47Herd::ReconcileMode()
{
 const FVector Offset=S47FarmWorld::HerdOffset(M);
 if(bModeKnown&&ModeOffset.Equals(Offset,1e-9))return false;
 ClearLife();
 const bool WasFreeform=!ModeOffset.IsNearlyZero();
 TArray<FVector> Old;
 for(int32 I=0;I<Count();++I){const FVector P(COWS[I][0],Ys[I],COWS[I][1]);Old.Add(Group?Group.LocalToWorld(P):P+ModeOffset);}
 ModeOffset=Offset;bModeKnown=true;
 Group.SetPosition(GroupBase+ModeOffset);
 for(int32 I=0;I<Count();++I)
 {
  const double Before=Ys[I];Ys[I]=S47RoadTerrain::SurfaceY(COWS[I][0]+Offset.X,COWS[I][1]+Offset.Z);
  for(int32 E=0;E<2;++E)
  {
   const int32 K=I*2+E;EyeAt[K].Y+=Ys[I]-Before;
   // Exported eye sprites share the road's glow pool, not the herd root.
   // Move only the matched 18 cattle sprites, never the other roadside lights.
   if(EyeSprites[K].IsValid())EyeSprites[K]->SetRelativeLocation(S47Three::ToUE(EyeAt[K]+ModeOffset));
  }
  FVector Look=LookBases[I];Look.Y+=Ys[I]-S47RoadTerrain::SurfaceY(COWS[I][0],COWS[I][1]);LookAreas[I].SetPosition(Look);
 }
 FVector R=RingBase;R.Y+=Ys[4]-S47RoadTerrain::SurfaceY(COWS[4][0],COWS[4][1]);Ring.SetPosition(R);
 if(S47FarmWorld::Enabled(M)){bStill=false;Missing=-1;Ring.SetVisible(false);}
 else if(WasFreeform){bStill=bStoryStill;Missing=StoryMissing;Ring.SetVisible(bStoryRing);}
 for(int32 I=0;I<LookAreas.Num();++I)LookAreas[I].SetVisible(I!=Missing);
 bDirty=true;Place(LastT);
 for(int32 I=0;I<Count();++I)
 {
  const FVector P(COWS[I][0],Ys[I],COWS[I][1]);const FVector New=Group?Group.LocalToWorld(P):P+ModeOffset;
  MoveObstacle(Old[I],New,I==Missing?0:S47Herd::COW_R);
 }
 return true;
}
void FS47Herd::MoveObstacle(const FVector& Old,const FVector& New,double Radius)
{
 if(!M||!M->Web)return;
 auto Match=[&](double X,double Z){return FMath::Abs(X-Old.X)<.001&&FMath::Abs(Z-Old.Z)<.001;};
 if(M->World)M->World->MoveHerdObstacle(Area,Old,New,Radius);
 for(const TCHAR* A:{TEXT("saro"),TEXT("road"),TEXT("diner")})for(auto& C:M->Web->CollidersMutable(A))if(Match((C.MinX+C.MaxX)*.5,(C.MinZ+C.MaxZ)*.5))
 {const double DX=New.X-Old.X,DZ=New.Z-Old.Z;C.MinX+=DX;C.MaxX+=DX;C.MinZ+=DZ;C.MaxZ+=DZ;C.bEnabled=Radius>0;}
}

// Grazing a while, then a few unhurried steps to the next spot, on a clock of its own and never far from where it belongs.
FVector FS47Herd::WanderAt(int32 I, double T, bool& bWalking, FVector& Heading) const
{
 const auto Spot=[I](int32 Seg)
 {
  const double A=FMath::Frac(FMath::Sin(Seg*12.9898+I*78.233)*43758.5453)*2*UE_DOUBLE_PI,R=2+FMath::Frac(FMath::Sin(Seg*39.346+I*11.135)*24634.6345)*5;
  return FVector(FMath::Cos(A)*R,0,FMath::Sin(A)*R);
 };
 const double Clock=T/(34.+I*4.3)+I*.37;const int32 Seg=FMath::FloorToInt(Clock);const double U=Clock-Seg;
 const FVector A=Spot(Seg),B=Spot(Seg+1);
 bWalking=U<.34;
 Heading=(B-A).GetSafeNormal();
 const double S=FMath::Clamp(U/.34,0.,1.);
 return FMath::Lerp(A,B,S*S*(3-2*S));
}

void FS47Herd::SetAlarm(double K, const FVector& FromWorld)
{
 Alarm=FMath::Clamp(K,0.,1.);
 AlarmFrom=FromWorld-(Group?Group.LocalToWorld(FVector::ZeroVector):ModeOffset);
}

// The collider and the handle go where the cow has got to, without being rewritten every frame.
void FS47Herd::FollowColliders()
{
 if(ColliderAt.Num()!=Count()||Offsets.Num()!=Count())return;
 for(int32 I=0;I<Count();++I)
 {
  const FVector P(COWS[I][0]+Offsets[I].X,Ys[I],COWS[I][1]+Offsets[I].Z);
  const FVector New=Group?Group.LocalToWorld(P):P+ModeOffset;
  if(FVector2D::Distance(FVector2D(New.X,New.Z),FVector2D(ColliderAt[I].X,ColliderAt[I].Z))<.6)continue;
  MoveObstacle(ColliderAt[I],New,S47Herd::COW_R);ColliderAt[I]=New;
  if(LookAreas.IsValidIndex(I)&&LookBases.IsValidIndex(I))
  {FVector Look=LookBases[I]+FVector(Offsets[I].X,Ys[I]-S47RoadTerrain::SurfaceY(COWS[I][0],COWS[I][1]),Offsets[I].Z);LookAreas[I].SetPosition(Look);}
 }
}

// Back to their own places, when the mode changes: Story's cattle stand where Story left them.
void FS47Herd::ClearLife()
{
 if(ColliderAt.Num()==Count())for(int32 I=0;I<Count();++I)
 {
  const FVector P(COWS[I][0],Ys[I],COWS[I][1]);const FVector Base=Group?Group.LocalToWorld(P):P+ModeOffset;
  if(FVector2D::Distance(FVector2D(Base.X,Base.Z),FVector2D(ColliderAt[I].X,ColliderAt[I].Z))>.001)MoveObstacle(ColliderAt[I],Base,I==Missing?0:S47Herd::COW_R);
 }
 ColliderAt.Reset();Fled.Reset();Offsets.Reset();Yaws.Reset();Alarm=0;
}

// Herd.pose: body and head matrices in road-local terms.
void FS47Herd::Pose(int32 I, double T)
{
	const double X = COWS[I][0], Z = COWS[I][1], Graze = COWS[I][2];
	double Yaw, Pitch, Turn;
	if (bStill) { Yaw = -AZ; Pitch = 0.06; Turn = 0; }
	else
	{
		const double C = std::fmod(T / (13 + I * 1.9) + I * 0.37, 1.0);
		const double Up = S47RoadShape::Smooth(0, 0.07, C) * (1 - S47RoadShape::Smooth(0.3, 0.37, C));
		Yaw = -Graze + 0.09 * FMath::Sin(T * 0.045 + I * 1.7);
		Pitch = -1.22 * (1 - Up) + 0.05 * Up + 0.05 * FMath::Sin(T * 1.3 + I) * (1 - Up);
		Turn = 0.28 * FMath::Sin(T * 0.29 + I * 2.1) * (1 - Up) + 0.35 * FMath::Sin(T * 0.21 + I) * Up;
	}
	BodyP = FVector(X, Ys[I], Z);
	double BodyPitch = 0;
	if (!bStill && S47FarmWorld::Enabled(M) && I != S47Herd::MISSING && Ys.IsValidIndex(I))
	{
		bool bWalk = false; FVector Heading(0, 0, -1);
		const FVector Wander = WanderAt(I, T, bWalk, Heading);
		const FVector Fear = Fled.IsValidIndex(I) ? Fled[I] : FVector::ZeroVector;
		const FVector Off = Wander + Fear;
		const double Base = S47FarmWorld::FloorRoad(X + ModeOffset.X, Z + ModeOffset.Z);
		const double Here = S47FarmWorld::FloorRoad(X + ModeOffset.X + Off.X, Z + ModeOffset.Z + Off.Z);
		BodyP = FVector(X + Off.X, Ys[I] + (Here - Base), Z + Off.Z);
		if (Offsets.IsValidIndex(I)) Offsets[I] = Off;
		const bool bRun = Alarm > .05 && Fear.SizeSquared() < 16 * 16;
		if (Alarm > .05)
		{
			// Head up and turned about, always ready to go; running when it can still get further off.
			const FVector Away = FVector(BodyP.X, 0, BodyP.Z) - FVector(AlarmFrom.X, 0, AlarmFrom.Z);
			Heading = Away.GetSafeNormal();
			Pitch = FMath::Lerp(Pitch, .12, Alarm); Turn = FMath::Lerp(Turn, .55 * FMath::Sin(T * 3.1 + I * 2.), Alarm);
			if (bRun)
			{
				BodyP.Y += .09 * FMath::Abs(FMath::Sin(T * 7.5 + I));
				BodyPitch = .09 * FMath::Sin(T * 7.5 + I);
			}
		}
		else if (bWalk)
		{
			BodyP.Y += .022 * FMath::Abs(FMath::Sin(T * 3.4 + I));
			BodyPitch = .012 * FMath::Sin(T * 3.4 + I);
			Pitch = FMath::Lerp(Pitch, -.25, .8); Turn *= .4;
		}
		// Turning takes it a moment.
		const double Want = FMath::Atan2(-Heading.X, -Heading.Z);
		if (Yaws.IsValidIndex(I)) { Yaws[I] += FMath::UnwindRadians(Want - Yaws[I]) * FMath::Min(1., StepDt * (Alarm > .05 ? 4. : 1.2)); Yaw = Yaws[I]; }
		else Yaw = Want;
	}
	else if (!bStill && I == S47Herd::MISSING && Alarm > .05 && S47FarmWorld::Enabled(M))
	{
		Pitch = FMath::Lerp(Pitch, .12, Alarm); Turn = FMath::Lerp(Turn, .55 * FMath::Sin(T * 3.1), Alarm);
	}
	BodyQ = S47Three::Euler(BodyPitch, Yaw, 0);
	HeadP = BodyP + BodyQ.RotateVector(S47Herd::NECK);
	HeadQ = BodyQ * S47Three::Euler(Pitch, Turn, 0, TEXT("YXZ"));
}

void FS47Herd::Place(double T)
{
	for (int32 I = 0; I < Count(); ++I)
	{
		if (I == Missing)
		{
			Bodies.SetInstance(I, FVector::ZeroVector, FQuat::Identity, FVector::ZeroVector, I == Count() - 1);
			Heads.SetInstance(I, FVector::ZeroVector, FQuat::Identity, FVector::ZeroVector, I == Count() - 1);
			continue;
		}
		Pose(I, T);
		Bodies.SetInstance(I, BodyP, BodyQ, FVector::OneVector, I == Count() - 1);
		Heads.SetInstance(I, HeadP, HeadQ, FVector::OneVector, I == Count() - 1);
	}
}

void FS47Herd::Set(bool bInStill, int32 InMissing, bool bRing)
{
 if(!S47FarmWorld::Enabled(M)){bStoryStill=bInStill;StoryMissing=InMissing;bStoryRing=bRing;}
 ReconcileMode();
 if(S47FarmWorld::Enabled(M)){bInStill=false;InMissing=-1;bRing=false;}
	if (bInStill != bStill || InMissing != Missing) { bStill = bInStill; Missing = InMissing; Place(LastT); }
	Ring.SetVisible(bRing);
	for (int32 I = 0; I < LookAreas.Num(); ++I) LookAreas[I].SetVisible(I != Missing);
	UpdateObstacles();
}

FS47Obstacle FS47Herd::Obstacle(const FVector2D& Origin)
{
 ReconcileMode();
	return FS47Obstacle::Circle(COWS[S47Herd::MISSING][0] + ModeOffset.X + Origin.X, COWS[S47Herd::MISSING][1] + ModeOffset.Z + Origin.Y,
		Missing == S47Herd::MISSING ? 0 : S47Herd::COW_R);
}

void FS47Herd::UpdateObstacles()
{
	if (!M || !M->Web) return;
	const FVector P = Group.LocalToWorld(FVector(COWS[S47Herd::MISSING][0], Ys[S47Herd::MISSING], COWS[S47Herd::MISSING][1]));
	// The native drive owns value arrays. Find the exported circle by its unchanged centre each
	// step, instead of holding a pointer into a TArray which the area's other builders can grow.
	if (M->World) M->World->MoveHerdObstacle(Area,P,P,Missing==S47Herd::MISSING?0:S47Herd::COW_R);
	for (const TCHAR* A : { TEXT("saro"), TEXT("road"), TEXT("diner") })
		for (FS47Rect& C : M->Web->CollidersMutable(A))
			if (FMath::Abs((C.MinX + C.MaxX) * 0.5 - P.X) < 0.001 && FMath::Abs((C.MinZ + C.MaxZ) * 0.5 - P.Z) < 0.001)
				C.bEnabled = Missing != S47Herd::MISSING;
}

double FS47Herd::BeamAt(const FVector& P) const
{
	const FVector L = FVector(BeamPos.X, BeamPos.Y, BeamPos.Z) - P;
	const double Length = L.Size(), Dist = Length == 0 ? 1e-3 : Length;
	const double Cone = S47RoadShape::Smooth(0.68, 0.96, -FVector::DotProduct(L, BeamDir) / Dist);
	return BeamPos.W * Cone * 1.4 / (1 + Dist * Dist / 2500);
}

void FS47Herd::Update(double T, const TOptional<FS47HerdHeadlights>& Headlights)
{
 ReconcileMode();
	const double Elapsed = FMath::Clamp(T - LastT, 0., .2);
	LastT = T; StepDt = Elapsed;
	if (S47FarmWorld::Enabled(M))
	{
		if (Fled.Num() != Count())
		{
			Fled.Init(FVector::ZeroVector, Count()); Offsets.Init(FVector::ZeroVector, Count()); Yaws.SetNum(Count());
			ColliderAt.SetNum(Count());
			for (int32 I = 0; I < Count(); ++I)
			{
				Yaws[I] = -COWS[I][2];
				const FVector P(COWS[I][0], Ys[I], COWS[I][1]);
				ColliderAt[I] = Group ? Group.LocalToWorld(P) : P + ModeOffset;
			}
		}
		for (int32 I = 0; I < Count(); ++I)
		{
			if (Alarm > .05)
			{
				// Away from it, each its own way; they bunch and scatter rather than march.
				const FVector Now(COWS[I][0] + Fled[I].X, 0, COWS[I][1] + Fled[I].Z);
				FVector Dir = (Now - FVector(AlarmFrom.X, 0, AlarmFrom.Z)).GetSafeNormal();
				Dir = FQuat(FVector::UpVector, .35 * FMath::Sin(T * .7 + I * 1.9)).RotateVector(Dir);
				if (Fled[I].Size() < 16) Fled[I] += Dir * 3.4 * Alarm * Elapsed;
			}
			else if (!Fled[I].IsNearlyZero()) Fled[I] = FMath::VInterpConstantTo(Fled[I], FVector::ZeroVector, Elapsed, .28);
		}
	}
	if (!bStill || bDirty) { Place(T); bDirty = false; }
	if (S47FarmWorld::Enabled(M)) FollowColliders();
	if (Headlights)
	{
		const FVector4& F0 = Headlights->F0;
		BeamDir = FVector(Headlights->F2.X - F0.X, 0, Headlights->F2.Z - F0.Z);
		if (BeamDir.SizeSquared() > 1) BeamDir.Normalize(); else BeamDir = FVector(0, 0, -1);
		BeamPos = FVector4(F0.X - BeamDir.X * 4, F0.Y, F0.Z - BeamDir.Z * 4, F0.W > 0 ? Headlights->ColorR : 0);
	}
	else BeamPos.W = 0;
	// Herd.beamLit: the material's small custom expression is in Tools/webexport/import_web.py.
	for (const FS47Obj& O : { Bodies, Heads, Ring }) if (O.IsValid())
		// Authored PBR hide uses native moon/flashlight/headlights; frost retains its old shader.
		if (!bNativeModels || O == Ring)
		for (UMaterialInstanceDynamic* D : O.W->NodeMids(O.I))
		{
			D->SetVectorParameterValue(TEXT("BeamPos"), FLinearColor(BeamPos.X, BeamPos.Y, BeamPos.Z, BeamPos.W));
			D->SetVectorParameterValue(TEXT("BeamDir"), FLinearColor(BeamDir.X, BeamDir.Y, BeamDir.Z));
			D->SetScalarParameterValue(TEXT("BeamFlat"), O == Ring ? 1.f : 0.f);
		}
	const bool bWant = bStill && BeamPos.W > 0;
	if (!bWant && !bEyesLit) return;
	bEyesLit = bWant;
	for (int32 I = 0; I < Count(); ++I) for (int32 E = 0; E < 2; ++E)
	{
		double K = 0;
		if (bWant && I != Missing)
		{
			const FVector P = Group ? Group.LocalToWorld(EyeAt[I * 2 + E]) : EyeAt[I * 2 + E];
			const double Fx = BeamPos.X - P.X, Fz = BeamPos.Z - P.Z;
			const double Length = FMath::Sqrt(Fx * Fx + Fz * Fz), Fd = Length == 0 ? 1 : Length;
			K = BeamAt(P) * S47RoadShape::Smooth(0.55, 0.9, (Fx * FACING.X + Fz * FACING.Z) / Fd) * 1.6;
		}
		if (Eyes[I * 2 + E].IsValid()) Eyes[I * 2 + E]->SetVectorParameterValue(TEXT("Color"), FLinearColor(K * 0.55, K * 0.9, K * 0.7));
	}
}

bool FS47Herd::CowAt(int32 I, FVector& Out) const
{
	if (I < 0 || I >= Count() || I == Missing) return false;
	Out = FVector(COWS[I][0], Ys[I] + 1.1, COWS[I][1]);
	if (Group) Out = Group.LocalToWorld(Out);else Out+=ModeOffset;
	// Where its collider stands: within a step of the cow, and one place for everything that asks.
	if (ColliderAt.IsValidIndex(I)) { Out.X = ColliderAt[I].X; Out.Z = ColliderAt[I].Z; }
	return true;
}

double FS47Herd::LitAt(int32 I) const { FVector P; return CowAt(I, P) ? BeamAt(P) : 0; }
FVector FS47Herd::Middle() const
{
	const FVector P(S47RoadShape::HERD.X, S47RoadTerrain::SurfaceY(S47RoadShape::HERD.X+ModeOffset.X, S47RoadShape::HERD.Z+ModeOffset.Z) + 1, S47RoadShape::HERD.Z);
	return Group ? Group.LocalToWorld(P) : P+ModeOffset;
}

bool FS47Herd::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool b, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s herd: %s"), b ? TEXT("PASS") : TEXT("FAIL"), What)); bOk &= b; };
	FS47Herd H(nullptr, FString(), TEXT("road"));
	Check(H.Count() == 9 && S47Herd::Tags()[S47Herd::MISSING] == TEXT("25"), TEXT("nine Herefords, missing ALDERETE 25"));
	H.bStill = true; H.Pose(4, 0);
	const FVector P = H.HeadP; const FQuat Q = H.HeadQ;
	H.Pose(4, 947);
	Check(H.HeadP.Equals(P, 1e-10) && H.HeadQ.Equals(Q, 1e-10), TEXT("026 still pose does not move"));
	const FVector F = H.BodyQ.RotateVector(FVector(0, 0, -1));
	Check(F.Equals(FACING, 1e-10), TEXT("the body faces azimuth 026"));
	H.bStill = false; H.Pose(4, 0); const FQuat Graze = H.HeadQ; H.Pose(4, 7);
	Check(!H.HeadQ.Equals(Graze, 1e-4), TEXT("the grazing head moves on its own clock"));
	// Matrices sampled from the original Herd in node/three (Float32 InstancedMesh storage).
	double E[16]; S47Three::Compose(H.HeadP, H.HeadQ, FVector::OneVector, E);
	Check(FMath::Abs(E[0] - -0.44334670901298523) < 1e-7 && FMath::Abs(E[6] - 0.4123375415802002) < 1e-7
		&& H.HeadP.Equals(FVector(-31.06801414489746, -0.9288950562477112, 557.9743041992188), 4e-5), TEXT("grazing matrix at t=7 agrees with original three.js"));
	Check(H.Middle().Equals(FVector(-37, -1.1610001087188722, 556), 1e-8)
		&& H.EyeAt[8].Equals(FVector(-30.042487525003317, -0.7795167454258476, 556.174161169384), 1e-8), TEXT("original terrain height, pasture middle and still eye placement"));
	H.Set(true, S47Herd::MISSING, true); FVector Out;
	Check(!H.CowAt(S47Herd::MISSING, Out) && H.CowAt(0, Out) && H.LitAt(S47Herd::MISSING) == 0, TEXT("the ninth and its beam sample disappear"));
	Check(H.Obstacle(FVector2D(8000, 0)).R == 0, TEXT("missing cow does not stop the truck"));
 // An asset-free relocation sample checks all nine photo/pose coordinates
 // against the same offset and terrain basis, without altering Story samples.
 FS47Herd Farm(nullptr,FString(),TEXT("road"));Farm.ModeOffset=FVector(92,0,-316);
 bool All=true;for(int32 I=0;I<Farm.Count();++I)
 {
  Farm.Ys[I]=S47RoadTerrain::SurfaceY(COWS[I][0]+92,COWS[I][1]-316);Farm.Pose(I,7);
  FVector At;All&=Farm.CowAt(I,At)&&FMath::IsNearlyEqual(At.X,COWS[I][0]+92)&&FMath::IsNearlyEqual(At.Z,COWS[I][1]-316)&&FMath::IsNearlyEqual(At.Y,Farm.BodyP.Y+1.1);
 }
 Check(All&&FMath::IsNearlyEqual(Farm.Middle().X,55.)&&FMath::IsNearlyEqual(Farm.Middle().Z,240.),TEXT("all nine relocated cattle/photo centres and pasture middle use the same road offset and local terrain"));
	H.BeamPos = FVector4(0, 0, 0, 1); H.BeamDir = FVector(0, 0, -1);
	Check(FMath::Abs(H.BeamAt(FVector(0, 0, -50)) - 0.7) < 1e-12 && H.BeamAt(FVector(0, 0, 50)) == 0, TEXT("exact cone and 2500 metre-squared beam falloff"));
	return bOk;
}

#include "S47Drive.h"

#include "S47Legs.h"
#include "S47OldRoad.h"
#include "S47RoadArea.h"
#include "S47RoadShape.h"
#include "S47Three.h"
#include "S47Truck.h"

namespace
{
	constexpr double WHEELBASE = 3.3;
	constexpr double MAX_STEER = 32 * UE_DOUBLE_PI / 180;
	constexpr double LF = 1.8, LR = WHEELBASE - LF;          // centre of the truck to the front and rear axles
	constexpr double HALF_W = 1.0, HALF_L = 2.65, TRACK = 0.84;
	constexpr double ACCEL = 3.5, BRAKE = 8, REVERSE = 5, LAT = 7.5;
	constexpr double RATIOS[3] = { 2.48, 1.48, 1.0 }, FINAL = 3.08, IDLE = FS47DriveController::IDLE;
	constexpr double YAW_MAX = 110 * UE_DOUBLE_PI / 180, PITCH0 = -0.08;

	double RoughOf(const FS47Surface& S)
	{
		if (S.Rough >= 0) return S.Rough;
		return S.Kind == ES47SurfaceKind::Asphalt ? 0.05 : S.Kind == ES47SurfaceKind::Gravel ? 0.4 : 0.8;
	}
	double Sign(double V) { return V > 0 ? 1.0 : V < 0 ? -1.0 : 0.0; }
}

const TCHAR* S47SurfaceName(ES47SurfaceKind K)
{
	return K == ES47SurfaceKind::Asphalt ? TEXT("asphalt") : K == ES47SurfaceKind::Gravel ? TEXT("gravel") : TEXT("dirt");
}

/** Full lock at a speed on ground of this grip: less as the speed rises, so the truck holds to
 *  about LAT m/s² sideways, and less again on loose ground. Physics, the drawn wheels and the
 *  autopilot (SteerFor) all turn the wheel through this one limit. */
double FS47DriveController::LockAt(double InSpeed, double Grip)
{
	const double V = FMath::Max(1.0, FMath::Abs(InSpeed));
	return FMath::Min(MAX_STEER, FMath::Atan(WHEELBASE * LAT * (0.45 + 0.55 * Grip) / (V * V)));
}

FS47DriveController::FS47DriveController(FS47Truck* InTruck, IS47DriveArea* InArea)
	: Rpm(IDLE), Truck(InTruck), Area(InArea), Pitch(PITCH0)
{
	if (Area && Area->Headlights && Truck) Truck->HeadlightFloods(Area->Headlights->Set, Area->Headlights->Slots);
}

void FS47DriveController::SetLook(double InYaw, double InPitch)
{
	Yaw = InYaw; Pitch = InPitch; Idle = 0;
	Pose(0, Steer * LockAt(Speed, Surface.Grip), false);
}

double FS47DriveController::SteerFor(double Kappa) const
{
	const double Beta = FMath::Asin(FMath::Clamp(LR * Kappa, -1.0, 1.0));
	const double Wheel = FMath::Atan(WHEELBASE / LR * FMath::Tan(Beta));
	return FMath::Clamp(Wheel / LockAt(Speed, Surface.Grip), -1.0, 1.0);
}

void FS47DriveController::Place(const FVector& P, double InHeading)
{
	Pos = FVector(P.X, 0, P.Z);
	Heading = InHeading;
	Speed = 0; Steer = 0; bReverse = false; Gear = 1; Accel = 0; YawRate = 0;
	Yaw = 0; Pitch = PITCH0; bArrived = false; bEngine = true; Power = 1; Held = 0; LookBox.Reset();
	const FGround G = Ground();
	Body = FBody();
	Body.Y = G.Y; Body.Pitch = G.Pitch; Body.Roll = G.Roll;
	Surface = Area->Surface(Pos.X, Pos.Z);
	if (Truck) Truck->SetDriving(true);
	Pose(0, 0);
}

void FS47DriveController::Adopt(const FS47DriveController& O, double X, double Z, double InHeading)
{
	Place(FVector(X, 0, Z), InHeading);
	Speed = O.Speed; Steer = O.Steer; bReverse = O.bReverse; Gear = O.Gear; Rpm = O.Rpm; Load = O.Load;
	Accel = O.Accel; YawRate = O.YawRate; Yaw = O.Yaw; Pitch = O.Pitch; Idle = O.Idle; T = O.T; bEngine = O.bEngine; Power = O.Power;
	// the body sits on its springs as it did: the same give over the new ground
	const FGround Og = O.Ground();
	FBody& B = Body;
	B.Y += O.Body.Y - Og.Y; B.Pitch += O.Body.Pitch - Og.Pitch; B.Roll += O.Body.Roll - Og.Roll;
	B.VY = O.Body.VY; B.VP = O.Body.VP; B.VR = O.Body.VR;
	Mph = O.Mph;
	Pose(0, 0, false);   // drawn as it is, the springs left as they were
}

void FS47DriveController::Update(double Dt, const FS47DriveIn& Input, const FVector2D& LookIn)
{
	T += Dt;
	const double Throttle = FMath::Clamp(Input.Throttle, -1.0, 1.0);
	// ---------- steering: the wheel turns at a limited rate, less lock at speed ----------
	const double Want = FMath::Clamp(Input.Steer, -1.0, 1.0);
	const double Rate = FMath::Abs(Want) > FMath::Abs(Steer) && Sign(Want) == Sign(Steer != 0 ? Steer : Want) ? 2.2 : 3.6;
	Steer += FMath::Clamp(Want - Steer, -Rate * Dt, Rate * Dt);
	// ---------- longitudinal ----------
	const FS47Surface S = Surface = Area->Surface(Pos.X, Pos.Z);
	const double V = Speed, Dir = bReverse ? -1 : 1;
	const double Go = Throttle * Dir;                 // > 0: pull in the selected direction (D or R)
	const bool bPulling = bEngine && Go > 0;
	const bool bBraking = bEngine ? Go < 0 : Throttle < 0;
	double A = 0;
	if (bPulling)
	{
		// the pull fades towards 1.19 x top, where it just matches drag at `top`
		const double Top = (bReverse ? REVERSE : S.Top) * 1.19;
		A = Dir * ACCEL * Go * Power * (0.55 + 0.45 * S.Grip) * FMath::Max(0.0, 1 - FMath::Square(FMath::Abs(V) / Top));
	}
	else if (bBraking)
	{
		A = -Sign(V) * BRAKE * FMath::Abs(Throttle) * (0.5 + 0.5 * S.Grip);
	}
	// a stopped truck held on the brake for a moment changes direction (D to R and back)
	if (bEngine && bBraking && FMath::Abs(V) < 0.3)
	{
		ShiftHold += Dt;
		if (ShiftHold > 0.3) { bReverse = !bReverse; ShiftHold = 0; }
	}
	else ShiftHold = 0;
	// rolling resistance (more on loose ground), air, engine braking, soft ground above its top speed
	const double Rough = RoughOf(S);
	double Drag = 0.18 + 0.5 * Rough + 0.0009 * V * V + (!bPulling && bEngine ? 0.25 : 0) + Held;
	if (FMath::Abs(V) > S.Top) Drag += (FMath::Abs(V) - S.Top) * 0.45;
	A -= Sign(V) * Drag;
	Accel = A;
	// ---------- move in small steps, so nothing is skipped at speed ----------
	const int32 N = FMath::Min(8, FMath::Max(1, int32(FMath::CeilToDouble(FMath::Abs(V) * Dt / 0.25))));
	const double H = Dt / N;
	for (int32 i = 0; i < N; ++i)
	{
		const double Before = Speed;
		Speed += A * H;
		// brakes and drag stop the truck; they never push it the other way
		if (!bPulling && Before != 0 && Sign(Speed) != Sign(Before)) { Speed = 0; A = 0; }
		const double Beta = FMath::Atan(LR / WHEELBASE * FMath::Tan(Steer * LockAt(Speed, S.Grip)));
		const double Ox = Pos.X, Oz = Pos.Z, Oh = Heading;
		YawRate = -Speed / LR * FMath::Sin(Beta);
		Heading += YawRate * H;
		// the centre moves a little to the inside of the heading (the slip angle beta)
		Pos.X -= FMath::Sin(Heading - Beta) * Speed * H;
		Pos.Z -= FMath::Cos(Heading - Beta) * Speed * H;
		if (Hit())
		{
			const double Impact = FMath::Abs(Speed);
			Pos.X = Ox; Pos.Z = Oz; Heading = Oh;
			Speed = -Sign(Speed) * FMath::Min(1.2, Impact * 0.15);
			if (Impact > 0.8 && BumpT <= 0) { BumpT = 0.6; if (OnBump) OnBump(Impact); }
			break;
		}
	}
	BumpT -= Dt;
	Mph = FMath::Abs(Speed) * 2.23694;
	// ---------- a three-speed automatic, for the sound and the dash ----------
	const double Fwd = FMath::Max(0.0, Speed);
	const double Pull = bPulling ? Go : 0;
	const double Up[2] = { 4.5 + 8 * Pull, 10 + 12 * Pull };
	if (bReverse) Gear = 1;
	else if (Gear < 3 && Fwd > Up[Gear - 1]) Gear++;
	else if (Gear > 1 && Fwd < Up[Gear - 2] - 3.5) Gear--;
	const double WheelRpm = FMath::Abs(Speed) * 60 / (2 * UE_DOUBLE_PI * 0.37);
	const double Coupled = WheelRpm * (bReverse ? 2.08 : RATIOS[Gear - 1]) * FINAL;
	const double Slip = (IDLE + Pull * 1500) * FMath::Max(0.0, 1 - FMath::Abs(Speed) / 7);
	// a misfire drags the revs down with the pull
	const double Target = bEngine ? FMath::Max3(IDLE * (0.55 + 0.45 * Power), Coupled, Slip * Power) : 0;
	Rpm += (Target - Rpm) * (1 - FMath::Exp(-Dt * (bEngine ? 6 : 1.6)));
	Load = bEngine ? Pull * Power : 0;
	// ---------- arrival ----------
	const FS47Box2& Z = Area->EndZone;
	if (!bArrived && Pos.X >= Z.MinX && Pos.X <= Z.MaxX && Pos.Z >= Z.MinZ && Pos.Z <= Z.MaxZ && FMath::Abs(Speed) < 6)
	{
		bArrived = true;
		if (OnArrive) OnArrive();
	}
	// ---------- look: free head within limits, eased back to the road when driving on ----------
	if (LookIn.X != 0 || LookIn.Y != 0) Idle = 0; else Idle += Dt;
	Yaw = FMath::Clamp(Yaw - LookIn.X, -YAW_MAX, YAW_MAX);
	Pitch = FMath::Clamp(Pitch - LookIn.Y, -0.9, 0.6);
	if (LookBox) { Yaw = FMath::Clamp(Yaw, LookBox->Yaw0, LookBox->Yaw1); Pitch = FMath::Clamp(Pitch, LookBox->Pitch0, LookBox->Pitch1); }
	if (Speed > 2 && Idle > 0.8)
	{
		const double K = 1 - FMath::Exp(-Dt * 1.4 * FMath::Min(1.0, Speed / 8));
		Yaw += (0 - Yaw) * K; Pitch += (PITCH0 - Pitch) * K;
	}
	if (Truck) Truck->SetBrake(bBraking);
	Pose(Dt, Steer * LockAt(Speed, S.Grip));
}

// Heights under the four wheels give the body its height, pitch and roll.
FS47DriveController::FGround FS47DriveController::Ground() const
{
	FGround G;
	if (!Area || !Area->HasHeight()) return G;
	const double Sx = -FMath::Sin(Heading), Sz = -FMath::Cos(Heading); // forward
	const double Rx = FMath::Cos(Heading), Rz = -FMath::Sin(Heading);  // right
	auto At = [&](double F, double R) { return Area->Height(Pos.X + Sx * F + Rx * R, Pos.Z + Sz * F + Rz * R); };
	const double Fl = At(LF, -TRACK), Fr = At(LF, TRACK), Rl = At(-LR, -TRACK), Rr = At(-LR, TRACK);
	const double Front = (Fl + Fr) / 2, Rear = (Rl + Rr) / 2;
	G.Y = Rear + (Front - Rear) * LR / WHEELBASE;
	G.Pitch = FMath::Atan2(Front - Rear, WHEELBASE);
	G.Roll = FMath::Atan2((Fr + Rr - Fl - Rl) / 2, 2 * TRACK);
	G.At[0] = Fl; G.At[1] = Fr; G.At[2] = Rl; G.At[3] = Rr;
	return G;
}

// Overlap of the truck's footprint (an oriented rectangle) with any obstacle.
bool FS47DriveController::Hit() const
{
	const double C = FMath::Cos(Heading), Sn = FMath::Sin(Heading), X = Pos.X, Z = Pos.Z;
	if (Area->Blocked(X, Z)) return true;
	for (const FS47Obstacle& O : Area->Obstacles)
	{
		if (O.bCircle)
		{
			const double Dx = O.X - X, Dz = O.Z - Z;
			if (Dx * Dx + Dz * Dz > FMath::Square(O.R + 2.9)) continue;
			const double Lx = Dx * C - Dz * Sn, Lz = Dx * Sn + Dz * C;
			const double Qx = Lx - FMath::Clamp(Lx, -HALF_W, HALF_W), Qz = Lz - FMath::Clamp(Lz, -HALF_L, HALF_L);
			if (Qx * Qx + Qz * Qz < O.R * O.R) return true;
		}
		else
		{
			const double Bx = (O.MinX + O.MaxX) / 2, Bz = (O.MinZ + O.MaxZ) / 2, Hx = (O.MaxX - O.MinX) / 2, Hz = (O.MaxZ - O.MinZ) / 2;
			const double Ac = FMath::Abs(C), As = FMath::Abs(Sn), Dx = Bx - X, Dz = Bz - Z;
			if (FMath::Abs(Dx) > Hx + HALF_W * Ac + HALF_L * As) continue;
			if (FMath::Abs(Dz) > Hz + HALF_W * As + HALF_L * Ac) continue;
			if (FMath::Abs(Dx * C - Dz * Sn) > HALF_W + Hx * Ac + Hz * As) continue;
			if (FMath::Abs(Dx * Sn + Dz * C) > HALF_L + Hx * As + Hz * Ac) continue;
			return true;
		}
	}
	return false;
}

// Body on its springs, then the camera at the driver's eye. With dt 0 the body settles on
// its springs (Place()), unless settle is false (Adopt(): it keeps its give and speed).
void FS47DriveController::Pose(double Dt, double WheelAngle, bool bSettle)
{
	const FGround G = Ground();
	FBody& B = Body;
	const double Rough = RoughOf(Surface);
	const double Fast = FMath::Min(1.0, FMath::Abs(Speed) / 12);
	const double Tt = T;
	// bumps: a few unrelated sines, scaled by surface and speed
	const double Bump = Rough * Fast * 0.025 * (FMath::Sin(Tt * 13.1) * 0.5 + FMath::Sin(Tt * 21.7 + 1.1) * 0.3 + FMath::Sin(Tt * 34.3 + 2.3) * 0.2);
	const double LatAcc = -Speed * YawRate; // positive in a right turn: the body leans left
	const double Tp = G.Pitch + FMath::Clamp(Accel * 0.0025, -0.03, 0.015) + Bump * 0.6;
	const double Tr = G.Roll + FMath::Clamp(LatAcc * 0.005, -0.05, 0.05);
	auto Spring = [&](double& X, double& Vx, double Target)
	{
		if (Dt <= 0) { if (bSettle) { X = Target; Vx = 0; } return; }
		const double K = 140, C = 18;
		const double Acc = K * (Target - X) - C * Vx;
		Vx += Acc * Dt; X = X + Vx * Dt;
	};
	Spring(B.Y, B.VY, G.Y + Bump);
	Spring(B.Pitch, B.VP, Tp);
	Spring(B.Roll, B.VR, Tr);
	const FQuat Qb = S47Three::Euler(B.Pitch, Heading, B.Roll, TEXT("YXZ"));
	if (Truck)
	{
		Truck->SetPose(FVector(Pos.X, B.Y, Pos.Z), FVector(B.Pitch, Heading, B.Roll));
		Truck->SetDriving(true);
		Truck->SetSteer(Steer);
		// the wheels stay on the ground while the body rides its springs: each one hangs as far
		// below its place on the body as the ground is (fl, fr, rl, rr)
		const double Sr = FMath::Sin(B.Roll) * FMath::Cos(B.Pitch), Sp = FMath::Sin(B.Pitch);
		double Drops[4];
		for (int32 i = 0; i < 4; ++i) Drops[i] = G.At[i] - (B.Y + ((i % 2) ? TRACK : -TRACK) * Sr - (i < 2 ? -LF : LR) * Sp);
		Truck->SetWheelDrops(Drops);
		Truck->Update(Dt, Speed, WheelAngle);
	}
	// camera: the eye, a little engine shake and road vibration
	const double Eng = bCameraMotion && Rpm > 50 ? (0.0004 + 0.0008 * FMath::Min(1.0, Rpm / 3000)) * (0.5 + Load) : 0;
	const double Road = bCameraMotion ? (Rough * 0.01 + 0.001) * Fast : 0;
	FVector V = Truck ? Truck->DriverEye : FVector(-0.42, 1.48, 0.2);
	V.Y += FMath::Sin(Tt * 47.3) * Eng + FMath::Sin(Tt * 17.9 + 0.7) * Road * 0.5;
	V.X += FMath::Sin(Tt * 23.1 + 2.1) * Road * 0.3;
	// Keep the physical truck on its springs; the comfort view follows the ground without sway.
	const FQuat EyeBody = bCameraMotion ? Qb : S47Three::Euler(0, Heading, 0, TEXT("YXZ"));
	CamPos = EyeBody.RotateVector(V) + FVector(Pos.X, bCameraMotion ? B.Y : G.Y, Pos.Z);
	const double Jitter = (FMath::Sin(Tt * 19.3) * 0.6 + FMath::Sin(Tt * 29.9 + 1.7) * 0.4) * Road * 0.5;
	const FQuat Qh = S47Three::Euler(Pitch + Jitter, Yaw, 0, TEXT("YXZ"));
	CamQuat = EyeBody * Qh;
}

// ---------- the self tests ----------

bool S47Drive::SelfTest(TArray<FString>& Lines)
{
	bool bOk = true;
	auto Check = [&](bool bPass, const FString& What) { Lines.Add(FString(bPass ? TEXT("ok   drive: ") : TEXT("FAIL drive: ")) + What); bOk &= bPass; };
	auto Near = [](double A, double B, double Tol = 1e-9) { return FMath::Abs(A - B) <= Tol; };
	auto Near3 = [&Near](const FVector& A, const FVector& B, double Tol) { return Near(A.X, B.X, Tol) && Near(A.Y, B.Y, Tol) && Near(A.Z, B.Z, Tol); };
	auto DinerOrigin = [] { const S47RoadShape::FDinerAt& D = S47RoadShape::DINER(); return FVector(8000 + D.X, D.Y + 0.1, D.Z); };
	Check(Near(FS47DriveController::LockAt(0, 1), 32 * UE_DOUBLE_PI / 180), TEXT("full lock at a standstill is 32 degrees"));
	{
		struct FComfortRoad : IS47DriveArea
		{
			virtual FS47Surface Surface(double, double) const override { return { ES47SurfaceKind::Gravel, 1, 30, .8 }; }
		} A;
		FS47DriveController Moving(nullptr, &A), Calm(nullptr, &A);
		Calm.bCameraMotion = false;
		Moving.Place(FVector::ZeroVector, 0); Calm.Place(FVector::ZeroVector, 0);
		for (int32 I = 0; I < 120; ++I) { Moving.Update(1. / 60., { .12, 1 }, FVector2D::ZeroVector); Calm.Update(1. / 60., { .12, 1 }, FVector2D::ZeroVector); }
		const FQuat LevelBody = S47Three::Euler(0, Calm.Heading, 0, TEXT("YXZ"));
		const FVector ExpectedEye = LevelBody.RotateVector(FVector(-.42, 1.48, .2)) + Calm.Pos;
		Check(Near3(Moving.Pos, Calm.Pos, 1e-12) && Near(Moving.Speed, Calm.Speed) && Near(Moving.Heading, Calm.Heading), TEXT("camera comfort preserves the vehicle's motion and handling"));
		Check(Near3(Calm.CamPos, ExpectedEye, 1e-9) && Calm.CamQuat.Equals(LevelBody * S47Three::Euler(Calm.Look().Y, Calm.Look().X, 0, TEXT("YXZ")), 1e-9), TEXT("comfort camera omits suspension sway, road vibration and engine shake"));
	}
	{
		// SteerFor is the inverse of the step: the curvature it asks for is the curvature driven
		FS47OldRoad A(FVector(8000, 0, 0));
		FS47DriveController D(nullptr, &A);
		D.Place(A.Start.Pos, A.Start.Heading);
		const double Kappa = 0.02, Steer = D.SteerFor(Kappa);
		const double Beta = FMath::Atan(1.5 / 3.3 * FMath::Tan(Steer * FS47DriveController::LockAt(D.Speed, D.Surface.Grip)));
		Check(Near(FMath::Sin(Beta) / 1.5, Kappa, 1e-9), TEXT("the autopilot's steer for a curvature (steerFor) is the bicycle step's inverse"));
	}
	// ---------- the web version's own answers to the same questions (the running game, 22 October 2026) ----------
	struct FPt { double X, Z, H; FS47Surface S; bool B; double G; };
	auto SameSurface = [&Near](const FS47Surface& A, const FS47Surface& B) { return A.Kind == B.Kind && Near(A.Grip, B.Grip) && Near(A.Top, B.Top) && Near(A.Rough, B.Rough); };
	{
		FS47RoadArea Road(FVector(8000, 0, 0));
		const FVector O = DinerOrigin();
		FS47Surface Lot; Lot.Kind = ES47SurfaceKind::Gravel; Lot.Grip = 0.75; Lot.Top = 20; Lot.Rough = 0.4;
		Road.Lots.Add({ { O.X + 3.6, O.X + 16, O.Z - 16, O.Z + 16 }, Lot, O.Y });
		const FPt Pts[] = {
			{ 8000.0, 300.0, -1.6472400840117127, { ES47SurfaceKind::Asphalt, 1.0, 30.0, 0.05 }, false, 0 },
			{ 7998.2, 495.0, -2.385615930222659, { ES47SurfaceKind::Asphalt, 1.0, 30.0, 0.05 }, false, 0 },
			{ 7990.0, 520.0, -2.360646990242021, { ES47SurfaceKind::Gravel, 0.75, 20.0, 0.4 }, false, 0 },
			{ 7900.0, 505.0, -1.5878383828281404, { ES47SurfaceKind::Dirt, 0.6, 12.0, 0.6 }, false, 0 },
			{ 7700.0, 497.0, -0.14529339589901952, { ES47SurfaceKind::Gravel, 0.75, 20.0, 0.4 }, false, 0 },
			{ 7520.0, 513.0, 0.3336661244091932, { ES47SurfaceKind::Dirt, 0.6, 12.0, 0.6 }, false, 0 },
			{ 7420.0, 530.0, -0.14877057566776877, { ES47SurfaceKind::Gravel, 0.75, 20.0, 0.4 }, false, 0 },
			{ 8030.0, 900.0, -2.58843731880188, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, false, 0 },
			{ 7985.0, 1999.0, 4.309006389843889, { ES47SurfaceKind::Gravel, 0.75, 20.0, 0.4 }, false, 0 },
			{ 8003.0, 2000.0, 4.305568889843889, { ES47SurfaceKind::Asphalt, 1.0, 30.0, 0.05 }, false, 0 },
			{ 8100.0, 1990.0, 3.4740392169952394, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, false, 0 },
			{ 8000.0, -100.0, -0.22953429630662103, { ES47SurfaceKind::Asphalt, 1.0, 30.0, 0.05 }, false, 0 },
			{ 8150.0, 1200.0, -0.34592832995414735, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, false, 0 },
			{ 7600.0, 300.0, -0.9422612190246582, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, false, 0 },
		};
		int32 Bad = 0;
		for (const FPt& P : Pts) { if (!Near(Road.Height(P.X, P.Z), P.H, 1e-4) || !SameSurface(Road.Surface(P.X, P.Z), P.S)) { Bad++; Lines.Add(FString::Printf(TEXT("     road at %.1f %.1f: %.5f (web %.5f)"), P.X, P.Z, Road.Height(P.X, P.Z), P.H)); } }
		Check(Bad == 0, TEXT("the road's height and surface at 14 points"));
		Check(Near3(Road.Start.Pos, FVector(7998.2, -2.3856159302226585, 495.0), 1e-4) && Near(Road.Start.Heading, 3.141592653589793), TEXT("the road's start"));
		Check(Near3(Road.End.Pos, FVector(7397.0, -0.29962555410858577, 532.0), 1e-4) && Near(Road.End.Heading, 1.5707963267948966), TEXT("the road's end"));
		Check(Road.Route.Num() == 104 && Near3(Road.Route[0], FVector(7998.2, 0.1115625, 0.0), 1e-4) && Near3(Road.Route[10], FVector(7998.2, 0.08261357274850199, 100.0), 1e-4) && Near3(Road.Route[60], FVector(7911.307499280659, -1.6287524630593568, 515.8608861396763), 1e-4) && Near3(Road.Route[103], FVector(7399.986887362203, -0.27753310583991136, 531.7724165523897), 1e-4), TEXT("the road's route to the gate (the survey track's points)"));
	}
	{
		const TArray<FVector>& R = S47Legs::RoadRoute(TEXT("station01"), -1e9);
		Check(R.Num() == 136 && Near3(R[0], FVector(7998.2, 0.0, -140.0), 1e-4) && Near3(R[33], FVector(7997.4, 0.0, 513.0), 1e-4) && Near3(R[40], FVector(7959.064000958715, 0.0, 519.6194725833084), 1e-4) && Near3(R[135], FVector(7394.0, 0.0, 531.9999999999999), 1e-4), TEXT("the autopilot's way to STATION 01"));
		const TArray<FVector>& D = S47Legs::RoadRoute(TEXT("diner"), -1e9);
		Check(D.Num() == 112 && Near(D.Last().X, 7991.0, 1e-4) && Near(D.Last().Z, 2005.95, 1e-4), TEXT("the autopilot's way to the diner"));
	}
	{
		FS47SaroDrive Saro({}, {});
		const FPt Pts[] = {
			{ 9.0, 8.0, 0.0, { ES47SurfaceKind::Asphalt, 1.0, 14.0, 0.08 }, false, 0 },
			{ 9.8, 14.0, -0.22857142857142848, { ES47SurfaceKind::Asphalt, 1.0, 14.0, 0.08 }, false, 0 },
			{ 10.0, 16.5, -0.5857142857142855, { ES47SurfaceKind::Asphalt, 1.0, 14.0, 0.08 }, false, 0 },
			{ -24.0, 30.0, -0.58, { ES47SurfaceKind::Asphalt, 1.0, 30.0, 0.05 }, false, 0 },
			{ -26.0, 300.0, 0.10902928963687915, { ES47SurfaceKind::Asphalt, 1.0, 30.0, 0.05 }, false, 0 },
			{ -26.0, 700.0, -2.068985051260685, { ES47SurfaceKind::Asphalt, 1.0, 30.0, 0.05 }, false, 0 },
			{ -36.0, 660.0, -2.098568384594018, { ES47SurfaceKind::Asphalt, 1.0, 14.0, 0.08 }, false, 0 },
			{ 3.0, 24.5, -0.6, { ES47SurfaceKind::Asphalt, 1.0, 14.0, 0.08 }, false, 0 },
			{ 20.0, 20.0, -0.6, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, true, 0 },
			{ -10.0, 100.0, -0.574816591763249, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, false, 0 },
			{ -22.0, 760.0, -2.2218421751264468, { ES47SurfaceKind::Asphalt, 1.0, 30.0, 0.05 }, false, 0 },
		};
		int32 Bad = 0;
		for (const FPt& P : Pts) { if (!Near(Saro.Height(P.X, P.Z), P.H, 1e-4) || !SameSurface(Saro.Surface(P.X, P.Z), P.S) || Saro.Blocked(P.X, P.Z) != P.B) { Bad++; Lines.Add(FString::Printf(TEXT("     saro at %.1f %.1f: %.5f (web %.5f)"), P.X, P.Z, Saro.Height(P.X, P.Z), P.H)); } }
		Check(Bad == 0, TEXT("SARO's ground: height, surface and the way at 11 points"));
		{ const TArray<FVector>* R = Saro.Routes.Find(TEXT("road")); Check(R && R->Num() == 68 && Near((*R)[0].X, 9.0, 1e-6) && Near((*R)[0].Z, 9.0, 1e-6) && Near((*R)[34].X, -25.8, 1e-6) && Near((*R)[34].Z, 140.0, 1e-6) && Near((*R)[67].X, -25.8, 1e-6) && Near((*R)[67].Z, 800.0, 1e-6), TEXT("SARO's route 'road'")); }
		{ const TArray<FVector>* R = Saro.Routes.Find(TEXT("park")); Check(R && R->Num() == 68 && Near((*R)[0].X, -22.2, 1e-6) && Near((*R)[0].Z, 800.0, 1e-6) && Near((*R)[34].X, -22.2, 1e-6) && Near((*R)[34].Z, 120.0, 1e-6) && Near((*R)[67].X, 9.0, 1e-6) && Near((*R)[67].Z, 8.4, 1e-6), TEXT("SARO's route 'park'")); }
		{ const TArray<FVector>* R = Saro.Routes.Find(TEXT("motel")); Check(R && R->Num() == 65 && Near((*R)[0].X, 9.0, 1e-6) && Near((*R)[0].Z, 9.0, 1e-6) && Near((*R)[32].X, -25.8, 1e-6) && Near((*R)[32].Z, 100.0, 1e-6) && Near((*R)[64].X, -37.3, 1e-6) && Near((*R)[64].Z, 660.2, 1e-6), TEXT("SARO's route 'motel'")); }
		{ const TArray<FVector>* R = Saro.Routes.Find(TEXT("motelS")); Check(R && R->Num() == 14 && Near((*R)[0].X, -22.2, 1e-6) && Near((*R)[0].Z, 794.8, 1e-6) && Near((*R)[7].X, -22.2, 1e-6) && Near((*R)[7].Z, 662.0, 1e-6) && Near((*R)[13].X, -37.3, 1e-6) && Near((*R)[13].Z, 660.2, 1e-6), TEXT("SARO's route 'motelS'")); }
		Check(Near(Saro.MotelPark.MinX, -43.0) && Near(Saro.MotelPark.MaxX, -31.0) && Near(Saro.MotelPark.MinZ, 653.0) && Near(Saro.MotelPark.MaxZ, 669.5), TEXT("where the truck is left at the motel"));
	}
	{
		FS47StationDrive St({}, {});
		const FPt Pts[] = {
			{ -19.0, 8031.0, -0.15000000596046448, { ES47SurfaceKind::Gravel, 0.75, 18.0, 0.4 }, false, -0.15000000596046448 },
			{ -19.0, 8060.0, -0.004341842514660327, { ES47SurfaceKind::Gravel, 0.75, 18.0, 0.4 }, false, -0.7109249383211136 },
			{ -10.0, 8130.0, 0.49369397734096176, { ES47SurfaceKind::Gravel, 0.75, 18.0, 0.4 }, false, -0.25756203573803566 },
			{ 10.0, 8250.0, 0.6302807603617392, { ES47SurfaceKind::Gravel, 0.75, 18.0, 0.4 }, false, -0.12010418076862994 },
			{ 17.0, 8350.0, 0.16694558260602813, { ES47SurfaceKind::Gravel, 0.75, 18.0, 0.4 }, false, -0.6036103420321013 },
			{ 12.0, 8440.0, -0.44943340513727237, { ES47SurfaceKind::Gravel, 0.75, 18.0, 0.4 }, false, -0.8895042948258576 },
			{ 40.0, 8200.0, 0.5668173338270658, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, true, -0.18586884565340558 },
			{ -5.0, 8010.0, -0.15000000596046448, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, true, -0.15000000596046448 },
			{ 0.0, 8100.0, 0.18993604803298703, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, true, -0.3865396991446989 },
			{ -100.0, 8300.0, -0.7845967346710645, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, true, -0.7845967346710645 },
		};
		int32 Bad = 0;
		for (const FPt& P : Pts) { if (!Near(St.Height(P.X, P.Z), P.H, 1e-4) || !Near(S47Legs::StationGroundAt(P.X, P.Z), P.G, 1e-4) || !SameSurface(St.Surface(P.X, P.Z), P.S) || St.Blocked(P.X, P.Z) != P.B) { Bad++; Lines.Add(FString::Printf(TEXT("     station at %.1f %.1f: %.5f (web %.5f), ground %.5f (web %.5f)"), P.X, P.Z, St.Height(P.X, P.Z), P.H, S47Legs::StationGroundAt(P.X, P.Z), P.G)); } }
		Check(Bad == 0, TEXT("STATION 01's ground: height, the station's terrain, surface and the way at 10 points"));
		{ const TArray<FVector>* R = St.Routes.Find(TEXT("road")); Check(R && R->Num() == 36 && Near((*R)[0].X, -19.25, 1e-6) && Near((*R)[0].Z, 8036.5, 1e-6) && Near((*R)[35].X, 11.7, 1e-6) && Near((*R)[35].Z, 8441.9, 1e-6), TEXT("STATION 01's route 'road'")); }
		{ const TArray<FVector>* R = St.Routes.Find(TEXT("park")); Check(R && R->Num() == 37 && Near((*R)[0].X, 11.7, 1e-6) && Near((*R)[0].Z, 8441.9, 1e-6) && Near((*R)[36].X, -19.25, 1e-6) && Near((*R)[36].Z, 8033.0, 1e-6), TEXT("STATION 01's route 'park'")); }
	}
	{
		FS47OldRoad Old(FVector(8000, 0, 0));
		const FVector O = DinerOrigin();
		Old.AddPlace({}, { O.X + 3.6, O.X + 16, O.Z - 16, O.Z + 16 }, O.Y);
		struct FOp { double X, Z, H; FS47Surface S; bool B; double Ws, Wo, Wd, Wmi, C; };
		const FOp Pts[] = {
			{ 8000.0, 2000.0, 4.329006389843889, { ES47SurfaceKind::Asphalt, 1.0, 30.0, 0.08 }, false, -13116.0, 0.0500000000001819, 0.0500000000001819, 7.518610113521618e-05, -6575.531960120582 },
			{ 8010.0, 1998.0, 4.407356086337179, { ES47SurfaceKind::Asphalt, 1.0, 30.0, 0.08 }, false, -13106.0, -1.949999999999818, 1.949999999999818, 0.006288913467631829, -6573.531960120582 },
			{ 8100.0, 1970.0, 3.6632661088144802, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, true, -13015.100476774742, -29.26395966230071, 29.263998349996438, 0.06277139897427375, -6545.531960120582 },
			{ 8500.0, 1880.0, 7.39511946583425, { ES47SurfaceKind::Asphalt, 1.0, 30.0, 0.08 }, false, -12594.207255490797, 1.7776997393785756, 1.777699740011704, 0.3243029717208312, -6455.531960120582 },
			{ 9500.0, 1400.0, 6.983083609459189, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, true, -11485.386134364275, -29.045573753673757, 29.045573809746998, 1.0132941862103255, -5975.531960120582 },
			{ 11000.0, 400.0, 7.103916801388645, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, true, -13116.121, 0.0, -1.0, 0.0, -4975.531960120582 },
			{ 16000.0, -1800.0, 2.0537155312085194, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, true, -4199.3235383697365, 84.78562699401743, 84.78562699401745, 5.540654840885248, -2775.531960120582 },
			{ 8000.0, 2200.0, 9.44564863243246, { ES47SurfaceKind::Asphalt, 1.0, 30.0, 0.08 }, false, -13116.121, 0.0, -1.0, 0.0, -6775.531960120582 },
			{ 8200.0, 1800.0, 1.0665980886840842, { ES47SurfaceKind::Dirt, 0.55, 10.0, 0.8 }, true, -13116.121, 0.0, -1.0, 0.0, -6375.531960120582 },
			{ 19080.0, -4575.0, 1.7994172506918036, { ES47SurfaceKind::Asphalt, 1.0, 30.0, 0.08 }, false, -0.5809079647832864, 0.09782161562098063, 0.09782161562098061, 8.149639039628182, -0.5319601205819708 },
		};
		int32 Bad = 0;
		for (const FOp& P : Pts)
		{
			const FS47OldRoad::FWhere Wh = Old.Where(P.X, P.Z);
			const bool bFar = !FMath::IsFinite(Wh.D);
			if (!Near(Old.Height(P.X, P.Z), P.H, 1e-4) || !SameSurface(Old.Surface(P.X, P.Z), P.S) || Old.Blocked(P.X, P.Z) != P.B || !Near(Wh.S, P.Ws, 1e-4) || !Near(Wh.O, P.Wo, 1e-4)
				|| (bFar ? P.Wd != -1 : !Near(Wh.D, P.Wd, 1e-4)) || !Near(Wh.Mi, P.Wmi, 1e-6) || !Near(Old.PastC(P.X, P.Z), P.C, 1e-6))
			{ Bad++; Lines.Add(FString::Printf(TEXT("     old road at %.1f %.1f: %.5f (web %.5f), s %.3f (web %.3f)"), P.X, P.Z, Old.Height(P.X, P.Z), P.H, Wh.S, P.Ws)); }
		}
		Check(Bad == 0, TEXT("the old road: height, surface, the right of way, where and C at 10 points"));
		Check(Near3(Old.Start.Pos, FVector(16477.61900810053, 2.085125250839815, -2136.7204515589765), 1e-4) && Near(Old.Start.Heading, -1.0995574287564276, 1e-9), TEXT("the old road's start, the right lane at 5.9 miles"));
		Check(Old.Route.Num() == 688 && Near3(Old.Route[0], FVector(7995.5, 4.276272014843889, 2003.9500000000007), 1e-4) && Near3(Old.Route[3], FVector(8010.0, 4.4122096019621715, 2001.5500000000002), 1e-4) && Near3(Old.Route[100], FVector(9540.863178952248, 7.379261120963141, 1415.9278529807152), 1e-4) && Near3(Old.Route[687], FVector(19245.606568411138, 2.649131772876429, -4808.891722489072), 1e-4), TEXT("the old road's route from the diner's lot"));
		Check(Near(Old.CrossY(), 1.6627402553836579, 1e-6), TEXT("the old road's height where C crosses it"));
		Check(Near3(Old.Lane(-5000), FVector(15255.385537073093, 2.4485562394431617, -1498.8500550984545), 1e-4), TEXT("the old road's right lane at s -5000"));
		Check(Near3(Old.Lane(0), FVector(19081.63311036354, 1.7934254741166158, -4574.713355219778), 1e-4), TEXT("the old road's right lane at s 0"));
		Check(Near3(Old.Lane(400), FVector(19311.063684903955, 2.346944780962859, -4902.374172935376), 1e-4), TEXT("the old road's right lane at s 400"));
		{ const FS47OldRoad::FPose Pa = Old.PoseAt(7.5); Check(Near3(Pa.Pos, FVector(18470.982626465535, 1.497996548647562, -3725.548528004757), 1e-4) && Near(Pa.Heading, -0.6974416566287168, 1e-9), TEXT("the truck's pose at mile 7.5")); }
	}
	{
		int32 Bad = 0;
		{ const TOptional<S47Legs::FExit> E = S47Legs::ExitFrom(TEXT("saro"), -24.0, 790.0, 3.141592653589793); if (!E || E->To != TEXT("road") || !Near(E->X, 8000.0) || !Near(E->Z, 490.0) || !Near(E->Heading, 3.141592653589793)) Bad++; }
		{ const TOptional<S47Legs::FExit> E = S47Legs::ExitFrom(TEXT("road"), 8001.0, 470.0, 0.0); if (!E || E->To != TEXT("saro") || !Near(E->X, -23.0) || !Near(E->Z, 770.0) || !Near(E->Heading, 0.0)) Bad++; }
		if (S47Legs::ExitFrom(TEXT("road"), 7700.0, 497.0, 1.3)) Bad++;
		if (S47Legs::ExitFrom(TEXT("station01"), 10.0, 8250.0, 3.0)) Bad++;
		if (S47Legs::ExitFrom(TEXT("station01"), 0.0, 8100.0, 0.0)) Bad++;
		if (S47Legs::ExitFrom(TEXT("saro"), -24.0, 700.0, 0.0)) Bad++;
		Check(Bad == 0, TEXT("the hand-overs between the areas (legs.exitFrom)"));
		Bad = 0;
		if (S47Legs::LegsBetween(TEXT("saro"), TEXT("station01")) != TArray<FString>{ TEXT("saro"), TEXT("road"), TEXT("station01") }) Bad++;
		if (S47Legs::LegsBetween(TEXT("station01"), TEXT("saro")) != TArray<FString>{ TEXT("station01"), TEXT("road"), TEXT("saro") }) Bad++;
		if (S47Legs::LegsBetween(TEXT("diner"), TEXT("saro")) != TArray<FString>{ TEXT("road"), TEXT("saro") }) Bad++;
		if (S47Legs::LegsBetween(TEXT("motel"), TEXT("diner")) != TArray<FString>{ TEXT("saro"), TEXT("road") }) Bad++;
		if (S47Legs::LegsBetween(TEXT("diner"), TEXT("roswell")) != TArray<FString>{ TEXT("diner"), TEXT("roswell") }) Bad++;
		Check(Bad == 0, TEXT("the legs a trip passes through (legs.legsBetween)"));
		Bad = 0;
		if (S47Legs::BoundOn(TEXT("road"), 7999.0, 300.0, 3.141592653589793, false) != TEXT("")) Bad++;
		if (S47Legs::BoundOn(TEXT("road"), 7999.0, 300.0, 0.0, false) != TEXT("saro")) Bad++;
		if (S47Legs::BoundOn(TEXT("road"), 7700.0, 497.0, 1.5707963267948966, false) != TEXT("station01")) Bad++;
		if (S47Legs::BoundOn(TEXT("saro"), -24.0, 300.0, 0.0, false) != TEXT("saro")) Bad++;
		if (S47Legs::BoundOn(TEXT("station01"), 0.0, 8200.0, 0.0, false) != TEXT("station01")) Bad++;
		if (S47Legs::BoundOn(TEXT("road"), 7999.0, 1000.0, 3.141592653589793, false) != TEXT("diner")) Bad++;
		Check(Bad == 0, TEXT("where a drive of one's own is bound (legs.boundOn)"));
	}
	{
		FS47RoadArea A(FVector(8000, 0, 0));
		const FVector O = DinerOrigin();
		FS47Surface Lot; Lot.Kind = ES47SurfaceKind::Gravel; Lot.Grip = 0.75; Lot.Top = 20; Lot.Rough = 0.4;
		A.Lots.Add({ { O.X + 3.6, O.X + 16, O.Z - 16, O.Z + 16 }, Lot, O.Y });
		const double StartX = A.Start.Pos.X, StartZ = A.Start.Pos.Z, StartH = A.Start.Heading;
		FS47DriveController D(nullptr, &A);
		const double Expect[][8] = {
			{ 60.0, 7998.2, 496.6750735101284, 3.141592653589793, 3.2855676428261207, 1295.768598924067, 1.0, 7.349617682943463 },
			{ 120.0, 7998.2, 501.59951807846625, 3.141592653589793, 6.490929665822339, 1184.6357487946204, 1.0, 14.519820206664624 },
			{ 180.0, 7996.553266899913, 509.4603952788047, 2.8534291990196707, 9.549175687314301, 1788.8889848594897, 1.0, 21.360933061980855 },
			{ 240.0, 7991.893369674154, 519.2660711222591, 2.631178532550834, 11.911500542360406, 2288.390660763846, 1.0, 26.64531202322769 },
			{ 300.0, 7986.9535693525095, 530.7017875685268, 2.821723693767574, 12.536231007389286, 1450.2490336112337, 3.0, 28.042796589669393 },
			{ 360.0, 7984.44781300375, 542.474822365204, 2.9512294292454677, 11.610386253928288, 934.8381058452288, 3.0, 25.971737426862347 },
			{ 420.0, 7982.332619505748, 553.4516343361563, 2.9512294292454677, 10.660879654373499, 865.670411675694, 3.0, 23.847748134054257 },
			{ 480.0, 7981.001008068153, 560.3620415318135, 2.9512294292454677, 3.562693204692563, 656.173744568154, 2.0, 7.969530937304982 },
			{ 540.0, 7980.84686857692, 561.1619495374729, 2.9512294292454677, -0.48752552107528363, 1670.3123218811324, 1.0, 1.090565339114145 },
		};
		D.Place(FVector(StartX, 0, StartZ), StartH);
		int32 Bad = 0, Row = 0;
		for (int32 i = 1; i <= 540; ++i)
		{
			FS47DriveIn In; In.Throttle = i < 300 ? 1 : i < 420 ? 0 : -1; In.Steer = i > 120 && i < 240 ? 0.35 : i >= 240 && i < 330 ? -0.5 : 0;
			D.Update(1.0 / 60, In, FVector2D::ZeroVector);
			if (i % 60 == 0 && Row < UE_ARRAY_COUNT(Expect))
			{
				const double* E = Expect[Row++];
				if (!Near(D.Pos.X, E[1], 1e-3) || !Near(D.Pos.Z, E[2], 1e-3) || !Near(D.Heading, E[3], 1e-5) || !Near(D.Speed, E[4], 1e-4) || !Near(D.Rpm, E[5], 0.05) || D.Gear != int32(E[6]))
				{ Bad++; Lines.Add(FString::Printf(TEXT("     road step %d: %.4f %.4f h %.5f v %.4f rpm %.1f gear %d (web %.4f %.4f h %.5f v %.4f rpm %.1f gear %d)"), i, D.Pos.X, D.Pos.Z, D.Heading, D.Speed, D.Rpm, D.Gear, E[1], E[2], E[3], E[4], E[5], int32(E[6]))); }
			}
		}
		Check(Bad == 0, TEXT("the controller on the road: 9 s of throttle, steering and brake, as the web version drives it"));
	}
	{
		FS47OldRoad A(FVector(8000, 0, 0));
		const FVector O = DinerOrigin();
		A.AddPlace({}, { O.X + 3.6, O.X + 16, O.Z - 16, O.Z + 16 }, O.Y);
		const double StartX = A.Start.Pos.X, StartZ = A.Start.Pos.Z, StartH = A.Start.Heading;
		FS47DriveController D(nullptr, &A);
		const double Expect[][8] = {
			{ 60.0, 16479.10496409286, -2137.4775839538943, -1.0995574287564276, 3.2709309925530494, 1299.548095023432, 1.0, 7.316876374481619 },
			{ 120.0, 16483.4731270608, -2139.7032741535986, -1.0995574287564276, 6.462089905230562, 1179.5092447505162, 1.0, 14.455307392606455 },
			{ 180.0, 16491.192487208762, -2141.7879076060813, -1.3887788623564892, 9.50741830946155, 1781.0444631452385, 1.0, 21.26752431316692 },
			{ 240.0, 16501.65600907228, -2142.159047581307, -1.5991018363545937, 10.977040265862211, 2142.5001031205384, 1.0, 24.554980452317817 },
			{ 300.0, 16512.65261785802, -2143.0648339264385, -1.3946832544050003, 10.83313340340182, 2075.4879642760798, 2.0, 24.23306943540567 },
			{ 360.0, 16522.43838626636, -2146.1184610891883, -1.249314972683433, 9.77223402908309, 791.5744613298427, 3.0, 21.85990118901713 },
			{ 420.0, 16527.94544110862, -2147.952498961329, -1.249314972683433, -0.8196556395936307, 661.1740669959792, 1.0, 1.8335204864325765 },
			{ 480.0, 16527.517002872082, -2147.8098143550937, -1.249314972683433, -1.3095392798065089, 1807.7166021475366, 1.0, 2.9293607965703723 },
			{ 540.0, 16525.361456287603, -2147.0919435177166, -1.249314972683433, -3.0983648849860312, 1274.628511129552, 1.0, 6.930856345820653 },
		};
		D.Place(FVector(StartX, 0, StartZ), StartH);
		int32 Bad = 0, Row = 0;
		for (int32 i = 1; i <= 540; ++i)
		{
			FS47DriveIn In; In.Throttle = i < 300 ? 1 : i < 420 ? 0 : -1; In.Steer = i > 120 && i < 240 ? 0.35 : i >= 240 && i < 330 ? -0.5 : 0;
			D.Update(1.0 / 60, In, FVector2D::ZeroVector);
			if (i % 60 == 0 && Row < UE_ARRAY_COUNT(Expect))
			{
				const double* E = Expect[Row++];
				if (!Near(D.Pos.X, E[1], 1e-3) || !Near(D.Pos.Z, E[2], 1e-3) || !Near(D.Heading, E[3], 1e-5) || !Near(D.Speed, E[4], 1e-4) || !Near(D.Rpm, E[5], 0.05) || D.Gear != int32(E[6]))
				{ Bad++; Lines.Add(FString::Printf(TEXT("     old road step %d: %.4f %.4f h %.5f v %.4f rpm %.1f gear %d (web %.4f %.4f h %.5f v %.4f rpm %.1f gear %d)"), i, D.Pos.X, D.Pos.Z, D.Heading, D.Speed, D.Rpm, D.Gear, E[1], E[2], E[3], E[4], E[5], int32(E[6]))); }
			}
		}
		Check(Bad == 0, TEXT("the controller on the old road, as the web version drives it"));
	}

	return bOk;
}

#include "S47Web.h"

#include "Components/LocalLightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "S47Walker.h"
#include "UnrealClient.h"

void AS47Web::ReviewLighting()
{
	FString Directory;
	if (!FParse::Value(FCommandLine::Get(), TEXT("S47LightingReview="), Directory) || LightingRunTime < 4) return;
	// One short run captures all review cameras. The flags never affect a normal game.
	struct FView { const TCHAR* Name; double X, Z, Yaw, Pitch; int32 Lab = -1; };
	static const FView Views[] = {
		{ TEXT("yard"), 9, -15, 3.14159, -0.05 },
		{ TEXT("east-door"), 10.5, 1.6, 1.5708, -0.1 },
		{ TEXT("lab-door"), 10, -2, -1.5708, -0.08 },
		{ TEXT("lot"), 7, 19, 0.08, 0.02 },
		{ TEXT("road"), -26, 2, -1.5708, 0.02 },
		{ TEXT("motel"), -39, 684, 0.7, -0.05 },
		{ TEXT("lab-normal"), 15.5, -1, 0, -0.12, 0 },
		{ TEXT("lab-safe"), 15.5, -1, 0, -0.12, 1 },
		{ TEXT("lab-print"), 15.5, -1, 0, -0.12, 2 }
	};
	const int32 Step = FMath::Min(int32((LightingRunTime - 4) / 3), int32(UE_ARRAY_COUNT(Views)) - 1);
	const FView& V = Views[Step];
	if (Step != LightingReviewStep)
	{
		TActorIterator<AS47Walker> It(GetWorld());
		if (It)
		{
			It->Place(V.X, V.Z, V.Yaw); It->Pitch = V.Pitch;
		}
		if (V.Lab >= 0)
		{
			SetDoorOpen(TEXT("lab"), V.Lab == 0, true);
			Light(TEXT("lab.normal")).SetIntensity(V.Lab == 0 ? 1.f : 0.f);
			Light(TEXT("lab.safelight")).SetIntensity(V.Lab != 0 ? 1.f : 0.f);
			Light(TEXT("lab.enlarger")).SetIntensity(V.Lab == 2 ? 1.f : 0.f);
		}
		LightingReviewStep = Step;
		UE_LOG(LogTemp, Display, TEXT("S47: lighting review camera %s at %.2f %.2f yaw=%.5f pitch=%.3f"), V.Name, V.X, V.Z, V.Yaw, V.Pitch);
	}
	if (LightingReviewShot != Step && LightingRunTime >= 4 + Step * 3 + 2.5)
	{
		if (V.Lab >= 0 && !LabLights.IsEmpty())
		{
			bool bPass = true;
			for (const FLabLight& P : LabLights) if (const ULocalLightComponent* L = P.Light.Get())
			{
				// White task light must stay off under safelight; the enlarger only exposes a print.
				if ((P.Role == 2 && V.Lab != 0) || (P.Role == 3 && V.Lab != 2) || ((P.Role == 0 || P.Role == 4) && V.Lab == 0)) bPass &= L->Intensity < 0.001f;
			}
			if (bPass) { UE_LOG(LogTemp, Display, TEXT("S47: lighting review lab mode %d PASS"), V.Lab); }
			else { UE_LOG(LogTemp, Error, TEXT("S47: lighting review lab mode %d FAIL"), V.Lab); }
		}
		FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory, FString(V.Name) + TEXT(".png")), true, false);
		LightingReviewShot = Step;
	}
	if (Step == 8 && LightingReviewShot == 8 && LightingRunTime > 31.5 && !bLightingReviewPower && !LabLights.IsEmpty())
	{
		// Exercise the existing power API without changing the story or leaving the scene altered.
		const int32 Lab = FloodSet(TEXT("lab"));
		const float Scale = FloodScaleOf(Lab);
		TArray<float> Before;
		for (const FLabLight& P : LabLights) Before.Add(P.Light.IsValid() ? P.Light->Intensity : 0.f);
		SetFloodScale(Lab, Scale * 0.5f); UpdateLabLighting();
		bool bPass = true;
		for (int32 I = 0; I < LabLights.Num(); ++I) if (LabLights[I].Light.IsValid()) bPass &= FMath::IsNearlyEqual(LabLights[I].Light->Intensity, Before[I] * 0.5f, 0.001f);
		SetFloodScale(Lab, 0.f); UpdateLabLighting();
		for (const FLabLight& P : LabLights)
		{
			if (P.Light.IsValid()) bPass &= P.Light->Intensity < 0.001f;
			if (P.Diffuser.IsValid())
			{
				const FLinearColor E = P.Diffuser->K2_GetVectorParameterValue(TEXT("Emissive"));
				bPass &= FMath::Max3(E.R, E.G, E.B) < 0.001f;
			}
		}
		SetFloodScale(Lab, Scale); UpdateLabLighting();
		if (bPass) { UE_LOG(LogTemp, Display, TEXT("S47: lighting review lab half-power, blackout and restore PASS")); }
		else { UE_LOG(LogTemp, Error, TEXT("S47: lighting review lab power FAIL")); }
		bLightingReviewPower = true;
	}
}

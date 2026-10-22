// Unreal-only original KELVORA props. The story keeps its existing nodes and material handles.
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/Texture.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Retro, Log, All);

void AS47Web::BuildRetroProps()
{
	UStaticMesh* Tv = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Kelvora/SM_Kelvora_TV.SM_Kelvora_TV"));
	UStaticMesh* Radio = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Kelvora/SM_Kelvora_Radio.SM_Kelvora_Radio"));
	if (!Tv || !Radio) { UE_LOG(LogS47Retro, Error, TEXT("S47: KELVORA assets missing")); return; }
	FString Text;
	TSharedPtr<FJsonObject> Manifest, Patches;
	FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("WebData/manifest.json")));
	FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Manifest);
	FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Kelvora/office-patches.json")));
	FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Patches);
	if (!Manifest || !Patches) { UE_LOG(LogS47Retro, Error, TEXT("S47: KELVORA office manifest missing")); return; }
	const int32 Office = Handle(TEXT("world.motel.tv.group"));
	if (!Nodes.IsValidIndex(Office)) return;
	// mergeStatic moved the old set into its parent's geometry. Substitute only the trimmed meshes;
	// leave all their existing material slots, collisions and visibility under the same scene nodes.
	for (FS47WebNode& N : Nodes)
	{
		if (N.Parent != Nodes[Office].Parent || N.Geo < 0) continue;
		const FString Name = FString::Printf(TEXT("g%d"), N.Geo);
		const TSharedPtr<FJsonObject>* Patch = nullptr;
		if (!Patches->TryGetObjectField(Name, Patch)) continue;
		if ((*Patch)->GetBoolField(TEXT("hidden"))) { SetVisible(int32(&N - Nodes.GetData()), false); continue; }
		UStaticMesh* Trimmed = LoadObject<UStaticMesh>(nullptr, *(TEXT("/Game/Kelvora/Office/") + Name + TEXT(".") + Name));
		UStaticMeshComponent* C = Cast<UStaticMeshComponent>(N.Comp.Get());
		if (C && Trimmed) C->SetStaticMesh(Trimmed);
		else { UE_LOG(LogS47Retro, Error, TEXT("S47: KELVORA office patch missing: %s"), *Name); return; }
	}
	auto Place = [this](int32 Anchor, UStaticMesh* Mesh, const TCHAR* Name, double Scale, const TCHAR* UseId)
	{
		if (!Nodes.IsValidIndex(Anchor)) return static_cast<UStaticMeshComponent*>(nullptr);
		const TArray<int32> Old = Nodes[Anchor].Children;
		for (int32 Child : Old)
		{
			SetVisible(Child, false);
			if (UPrimitiveComponent* P = Cast<UPrimitiveComponent>(Nodes[Child].Comp.Get())) P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, Name);
		C->SetMobility(EComponentMobility::Movable);
		C->SetupAttachment(Nodes[Anchor].Comp.Get());
		C->SetStaticMesh(Mesh);
		// FBX +X is the face; the old three.js group faces local +Z (Unreal +Y).
		C->SetRelativeRotation(FRotator(0, 90, 0));
		C->SetRelativeScale3D(FVector(Scale));
		C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		C->SetCollisionResponseToAllChannels(ECR_Ignore);
		C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		C->SetCastShadow(true);
		C->RegisterComponent();
		AdoptNode(C, Anchor, Name);
		const int32 I = NodeOf(C);
		MarkUsable(I, UseId);
		return C;
	};
	auto Bind = [this](UStaticMeshComponent* C, int32 MatId)
	{
		if (!C || MatId < 0) return;
		UMaterialInstanceDynamic* Previous = SharedMid(MatId);
		if (!Previous) return;
		// The set's own fill light must not reflect as a white spot over the programme.
		UMaterialInstanceDynamic* Screen = UMaterialInstanceDynamic::Create(C->GetMaterial(1), this);
		Screen->CopyParameterOverrides(Previous);
		Screen->SetVectorParameterValue(TEXT("EmU"), FLinearColor(1, 0, 0, 0));
		Screen->SetVectorParameterValue(TEXT("EmV"), FLinearColor(0, -1, 1, 0));
		C->SetMaterial(1, Screen);
		KeepMids.Add(Screen);
		// The original film writers address this id, including room 6's power and restored save state.
		SharedMidCache.Add(MatId, Screen);
		UMaterialInstanceDynamic* Pilot = UMaterialInstanceDynamic::Create(C->GetMaterial(2), this);
		C->SetMaterial(2, Pilot);
		KeepMids.Add(Pilot);
		RetroTvs.Add({Screen, Pilot});
		FTimerHandle Timer;
		GetWorldTimerManager().SetTimer(Timer, [Screen = TWeakObjectPtr<UMaterialInstanceDynamic>(Screen), C = TWeakObjectPtr<UStaticMeshComponent>(C)]
		{
			if (!Screen.IsValid() || !C.IsValid()) return;
			UTexture* Image = nullptr; float Enabled = 0; FLinearColor E = FLinearColor::Black;
			Screen->GetTextureParameterValue(FMaterialParameterInfo(TEXT("EmissiveMap")), Image);
			Screen->GetScalarParameterValue(FMaterialParameterInfo(TEXT("UseEmissiveMap")), Enabled);
			Screen->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Emissive")), E);
			UE_LOG(LogS47Retro, Display, TEXT("S47: KELVORA %s screen %s, image %s, enabled %.1f, emission %s, assigned %d"), *C->GetName(), *Screen->Parent->GetPathName(), Image ? *Image->GetName() : TEXT("none"), Enabled, *E.ToString(), C->GetMaterial(1) == Screen.Get());
		}, 5.f, false);
	};
	int32 OfficeMat = -1;
	for (const auto& V : Manifest->GetArrayField(TEXT("materials")))
	{
		double Tex = -1;
		if (V->AsObject()->TryGetNumberField(TEXT("emissiveMap"), Tex) && int32(Tex) == HandleTex(TEXT("world.motel.tv.tex")))
			OfficeMat = V->AsObject()->GetIntegerField(TEXT("id"));
	}
	Bind(Place(Office, Tv, TEXT("Kelvora_OfficeTV"), 1, TEXT("court:officeTv")), OfficeMat);
	Bind(Place(Handle(TEXT("world.room6.objs.tv")), Tv, TEXT("Kelvora_Room6TV"), 1.25, TEXT("r6:tv")), HandleMat(TEXT("world.room6.tvMaterial")));
	const bool bRadio = Place(Handle(TEXT("room.objs.radio")), Radio, TEXT("Kelvora_SaroRadio"), 1, TEXT("radio")) != nullptr;
	UE_LOG(LogS47Retro, Display, TEXT("S47: KELVORA placed %d televisions and %d radio; existing movie, power and audio handles retained"), RetroTvs.Num(), bRadio ? 1 : 0);
}

void AS47Web::UpdateRetroProps()
{
	for (const FRetroTv& Tv : RetroTvs)
	{
		if (!Tv.Screen.IsValid() || !Tv.Pilot.IsValid()) continue;
		FLinearColor Emission = FLinearColor::Black;
		Tv.Screen->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Emissive")), Emission);
		Tv.Pilot->SetVectorParameterValue(TEXT("Emissive"), FMath::Max3(Emission.R, Emission.G, Emission.B) > .01f ? FLinearColor(.8f, .002f, .001f) : FLinearColor::Black);
	}
}

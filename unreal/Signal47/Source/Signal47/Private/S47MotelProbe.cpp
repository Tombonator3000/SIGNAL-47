#include "S47MotelProbe.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47Three.h"
#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "StaticMeshResources.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#endif

namespace
{
	// The reception room's floor rectangle in the motel's three-space (the assembly's "reception-tiles").
	constexpr double ProbeRoomMinX = -48.106, ProbeRoomMaxX = -43.714, ProbeRoomMinZ = 672.304, ProbeRoomMaxZ = 677.496;

	/** Per triangle: does the winding's geometric normal agree with the stored vertex normals? */
	struct FProbeAudit
	{
		TArray<int8> Sign;
		TArray<FVector3f> Centre;
		TArray<int32> Slot;
		void Add(const FVector3f& A, const FVector3f& B, const FVector3f& C, const FVector3f& Na, const FVector3f& Nb, const FVector3f& Nc, int32 InSlot)
		{
			const FVector3f G = FVector3f::CrossProduct(B - A, C - A);
			const FVector3f N = Na + Nb + Nc;
			if (G.SizeSquared() < 1e-10f || N.SizeSquared() < 1e-6f) return;
			Sign.Add(FVector3f::DotProduct(G, N) > 0.f ? 1 : -1);
			Centre.Add((A + B + C) / 3.f);
			Slot.Add(InSlot);
		}
	};

	bool ProbeShown(const UStaticMeshComponent* C) { return C && C->IsVisible() && !C->bHiddenInGame; }

	void ProbeReport(const UStaticMeshComponent* C, int32 Lod, const TCHAR* Source, const FProbeAudit& A)
	{
		int64 Plus = 0;
		for (const int8 S : A.Sign) Plus += S > 0 ? 1 : 0;
		const int64 Total = A.Sign.Num(), Minus = Total - Plus;
		const int8 Major = Plus >= Minus ? 1 : -1;
		TMap<int32, int64> BySlot;
		TMap<FIntPoint, int64> Cells;
		int64 Minor = 0, InRoom = 0;
		const FTransform World = C->GetComponentTransform();
		for (int32 I = 0; I < A.Sign.Num(); ++I)
		{
			if (A.Sign[I] == Major) continue;
			++Minor;
			++BySlot.FindOrAdd(A.Slot[I]);
			const FVector P = S47Three::ToThree(World.TransformPosition(FVector(A.Centre[I])));
			++Cells.FindOrAdd(FIntPoint(FMath::FloorToInt(P.X), FMath::FloorToInt(P.Z)));
			if (P.X >= ProbeRoomMinX && P.X <= ProbeRoomMaxX && P.Z >= ProbeRoomMinZ && P.Z <= ProbeRoomMaxZ) ++InRoom;
		}
		TArray<TPair<FIntPoint, int64>> Sorted;
		for (const auto& Pair : Cells) Sorted.Add(TPair<FIntPoint, int64>(Pair.Key, Pair.Value));
		Sorted.Sort([](const TPair<FIntPoint, int64>& L, const TPair<FIntPoint, int64>& R) { return L.Value > R.Value; });
		FString Cell, Slots;
		for (int32 I = 0; I < FMath::Min(12, Sorted.Num()); ++I) Cell += FString::Printf(TEXT("[x%d z%d]=%lld "), Sorted[I].Key.X, Sorted[I].Key.Y, Sorted[I].Value);
		for (const auto& Pair : BySlot) Slots += FString::Printf(TEXT("slot%d=%lld "), Pair.Key, Pair.Value);
		UE_LOG(LogTemp, Display, TEXT("S47 office winding: %s lod=%d via %s tris=%lld majority=%s minority=%lld (%.3f%%) minority_in_reception_room=%lld by_slot: %s top_cells_1m: %s"),
			*C->GetStaticMesh()->GetName(), Lod, Source, Total, Major > 0 ? TEXT("cross(B-A,C-A) with normal") : TEXT("cross(B-A,C-A) against normal"),
			Minor, Total ? 100.0 * double(Minor) / double(Total) : 0.0, InRoom, *Slots, *Cell);
	}

	void ProbeAuditMesh(const UStaticMeshComponent* C)
	{
		const UStaticMesh* Mesh = C->GetStaticMesh();
		const int32 Lods = Mesh->GetRenderData() ? Mesh->GetRenderData()->LODResources.Num() : 0;
		for (int32 L = 0; L < Lods; ++L)
		{
			FProbeAudit A;
			const TCHAR* Source = TEXT("none");
#if WITH_EDITOR
			if (L < Mesh->GetNumSourceModels())
			{
				if (const FMeshDescription* D = Mesh->GetMeshDescription(L))
				{
					const FStaticMeshConstAttributes Attributes(*D);
					const auto Positions = Attributes.GetVertexPositions();
					const auto Normals = Attributes.GetVertexInstanceNormals();
					for (const FTriangleID Tri : D->Triangles().GetElementIDs())
					{
						const TArrayView<const FVertexInstanceID> V = D->GetTriangleVertexInstances(Tri);
						if (V.Num() != 3) continue;
						A.Add(Positions[D->GetVertexInstanceVertex(V[0])], Positions[D->GetVertexInstanceVertex(V[1])], Positions[D->GetVertexInstanceVertex(V[2])],
							Normals[V[0]], Normals[V[1]], Normals[V[2]], D->GetTrianglePolygonGroup(Tri).GetValue());
					}
					Source = TEXT("saved MeshDescription");
				}
			}
#endif
			if (A.Sign.IsEmpty())
			{
				const FStaticMeshLODResources& R = Mesh->GetRenderData()->LODResources[L];
				const FStaticMeshVertexBuffers& B = R.VertexBuffers;
				if (Mesh->bAllowCPUAccess && B.PositionVertexBuffer.GetVertexData() && B.StaticMeshVertexBuffer.GetTangentData() && R.IndexBuffer.GetIndexDataSize() > 0)
				{
					TArray<uint32> Idx;
					R.IndexBuffer.GetCopy(Idx);
					for (int32 S = 0; S < R.Sections.Num(); ++S)
					{
						const FStaticMeshSection& Section = R.Sections[S];
						for (uint32 I = Section.FirstIndex; I + 2 < uint32(Idx.Num()) && I < Section.FirstIndex + Section.NumTriangles * 3; I += 3)
						{
							const uint32 X = Idx[I], Y = Idx[I + 1], Z = Idx[I + 2];
							if (FMath::Max3(X, Y, Z) >= B.PositionVertexBuffer.GetNumVertices()) continue;
							A.Add(B.PositionVertexBuffer.VertexPosition(X), B.PositionVertexBuffer.VertexPosition(Y), B.PositionVertexBuffer.VertexPosition(Z),
								B.StaticMeshVertexBuffer.VertexTangentZ(X), B.StaticMeshVertexBuffer.VertexTangentZ(Y), B.StaticMeshVertexBuffer.VertexTangentZ(Z), S);
						}
					}
					Source = TEXT("render buffers");
				}
			}
			if (A.Sign.IsEmpty())
			{
				UE_LOG(LogTemp, Display, TEXT("S47 office winding: %s lod=%d unavailable (no saved MeshDescription and no CPU-readable render buffers in this build; run in the editor)"), *Mesh->GetName(), L);
				continue;
			}
			ProbeReport(C, L, Source, A);
		}
	}

	FString ProbeMaterialLine(const UStaticMeshComponent* C)
	{
		FString Out;
		for (int32 K = 0; K < C->GetNumMaterials(); ++K)
		{
			const UMaterialInterface* M = C->GetMaterial(K);
			Out += FString::Printf(TEXT("%d:%s(%s) "), K, M ? *M->GetName() : TEXT("none"), M ? (M->IsTwoSided() ? TEXT("two-sided") : TEXT("one-sided")) : TEXT("-"));
		}
		return Out;
	}
}

namespace S47MotelProbe
{
	void LogOffice(US47Main* Main)
	{
		AS47Web* W = Main ? Main->Web.Get() : nullptr;
		if (!W) { UE_LOG(LogTemp, Error, TEXT("S47 office probe: no web")); return; }
		int32 Count = 0;
		for (int32 I = 0; I < W->PartCount(); ++I)
		{
			const auto* N = W->Node(I);
			const UStaticMeshComponent* C = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
			if (!C || !C->GetStaticMesh()) continue;
			const FString Name = C->GetStaticMesh()->GetName();
			const bool Old = Name.StartsWith(TEXT("SM_Town_Motel_"));
			const bool Office = Name == TEXT("SM_Sierra_Office") || Name == TEXT("SM_Sierra_Glass") || Name == TEXT("SM_Sierra_RoofShell") || Name == TEXT("SM_Sierra_GlowOffice");
			if (!Old && !Office) continue;
			++Count;
			const FBox Box = C->Bounds.GetBox();
			const FVector Lo = S47Three::ToThree(Box.Min), Hi = S47Three::ToThree(Box.Max);
			UE_LOG(LogTemp, Display, TEXT("S47 office part: node=%d %s mesh=%s %s shown=%d lod_tris=%d/%d bounds_three=[%.2f %.2f %.2f]..[%.2f %.2f %.2f] materials: %s"),
				I, *N->Name, *Name, Old ? TEXT("RETIRED-NATIVE") : TEXT("V2"), ProbeShown(C) ? 1 : 0, C->GetStaticMesh()->GetNumTriangles(0), C->GetStaticMesh()->GetNumLODs(),
				Lo.X, Lo.Y, Lo.Z, Hi.X, Hi.Y, Hi.Z, *ProbeMaterialLine(C));
			if (Name == TEXT("SM_Sierra_Office") || Name == TEXT("SM_Sierra_RoofShell") || Name == TEXT("SM_Town_Motel_Office")) ProbeAuditMesh(C);
		}
		UE_LOG(LogTemp, Display, TEXT("S47 office probe: %d office-related components listed. A retired-native part with shown=1 would draw a second, overlapping wall set."), Count);
	}

	void LogRays(US47Main* Main, const TCHAR* Name)
	{
		AS47Web* W = Main ? Main->Web.Get() : nullptr;
		AS47Walker* P = Main ? Main->Player.Get() : nullptr;
		if (!W || !P) return;
		struct FDir { FString Label; double Yaw, Pitch; };
		TArray<FDir> Dirs;
		Dirs.Add({ FString(TEXT("forward")), P->Yaw, P->Pitch });
		for (int32 K = 0; K < 8; ++K) Dirs.Add({ FString::Printf(TEXT("fan%d"), K * 45), P->Yaw + K * UE_DOUBLE_PI / 4., 0. });
		Dirs.Add({ FString(TEXT("up")), P->Yaw, UE_DOUBLE_PI / 2 - .01 });
		Dirs.Add({ FString(TEXT("down")), P->Yaw, -UE_DOUBLE_PI / 2 + .01 });
		const FVector Eye(P->Pos.X, P->FloorY + P->Eye, P->Pos.Z);
		UE_LOG(LogTemp, Display, TEXT("S47 office rays %s: eye_three=[%.2f %.2f %.2f] yaw=%.3f pitch=%.3f"), Name, Eye.X, Eye.Y, Eye.Z, P->Yaw, P->Pitch);
		for (const FDir& D : Dirs)
		{
			const FVector Fwd(-FMath::Sin(D.Yaw) * FMath::Cos(D.Pitch), FMath::Sin(D.Pitch), -FMath::Cos(D.Yaw) * FMath::Cos(D.Pitch));
			const FVector Start = P->EyeUE(), End = S47Three::ToUE(Eye + Fwd * 40.);
			FHitResult H;
			FCollisionQueryParams Q(SCENE_QUERY_STAT(S47MotelProbeRay), true, P);
			Q.bReturnFaceIndex = true;
			if (!W->GetWorld()->LineTraceSingleByChannel(H, Start, End, ECC_Visibility, Q))
			{
				UE_LOG(LogTemp, Display, TEXT("S47 office ray %s %s: no hit within 40 m"), Name, *D.Label);
				continue;
			}
			UStaticMeshComponent* C = Cast<UStaticMeshComponent>(H.GetComponent());
			int32 Section = INDEX_NONE;
			UMaterialInterface* M = C ? C->GetMaterialFromCollisionFaceIndex(H.FaceIndex, Section) : nullptr;
			const int32 Node = W->NodeOf(H.GetComponent());
			const auto* N = W->Node(Node);
			UE_LOG(LogTemp, Display, TEXT("S47 office ray %s %s: %.2f m node=%d %s mesh=%s material=%s %s shown=%d normal_dot_ray=%.2f (collision normals are only indicative)"),
				Name, *D.Label, H.Distance / 100., Node, N ? *N->Name : TEXT("-"), C && C->GetStaticMesh() ? *C->GetStaticMesh()->GetName() : TEXT("-"),
				M ? *M->GetName() : TEXT("-"), M ? (M->IsTwoSided() ? TEXT("two-sided") : TEXT("one-sided")) : TEXT(""), ProbeShown(C) ? 1 : 0,
				FVector::DotProduct(H.ImpactNormal, (End - Start).GetSafeNormal()));
		}
	}

	void LogPool(US47Main* Main)
	{
		AS47Web* W = Main ? Main->Web.Get() : nullptr;
		if (!W) return;
		// Fence rectangle -64.9..-53.9 x 690.9..707.9 widened by 3 m. A lump at a fence post can also be something behind it.
		constexpr double MinX = -67.9, MaxX = -50.9, MinZ = 687.9, MaxZ = 710.9;
		struct FRow { double Size; FString Text; };
		TArray<FRow> Rows;
		auto Overlaps = [](const FVector& Lo, const FVector& Hi) { return Hi.X >= MinX && Lo.X <= MaxX && Hi.Z >= MinZ && Lo.Z <= MaxZ && Hi.Y > -2.9 && Lo.Y < 1.5; };
		auto Inside = [](const FVector& C) { return C.X > -64.9 && C.X < -53.9 && C.Z > 690.9 && C.Z < 707.9; };
		for (int32 I = 0; I < W->PartCount(); ++I)
		{
			const auto* N = W->Node(I);
			const UStaticMeshComponent* C = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
			if (!C || !C->GetStaticMesh() || !ProbeShown(C) || C->IsA<UInstancedStaticMeshComponent>()) continue;
			const FBox Box = C->Bounds.GetBox();
			if (!Box.IsValid) continue;
			const FVector Lo = S47Three::ToThree(Box.Min), Hi = S47Three::ToThree(Box.Max);
			if (!Overlaps(Lo, Hi)) continue;
			const UMaterialInterface* M = C->GetNumMaterials() > 0 ? C->GetMaterial(0) : nullptr;
			Rows.Add({ (Hi - Lo).Size(), FString::Printf(TEXT("mesh node=%d %s geo=%d parent=%d mesh=%s tris=%d mat0=%s size=[%.2f %.2f %.2f] min=[%.2f %.2f %.2f] max=[%.2f %.2f %.2f] centre_inside_fence=%d"),
				I, *N->Name, N->Geo, N->Parent, *C->GetStaticMesh()->GetName(), C->GetStaticMesh()->GetNumTriangles(0), M ? *M->GetName() : TEXT("-"),
				Hi.X - Lo.X, Hi.Y - Lo.Y, Hi.Z - Lo.Z, Lo.X, Lo.Y, Lo.Z, Hi.X, Hi.Y, Hi.Z, Inside((Lo + Hi) * .5) ? 1 : 0) });
		}
		TMap<FString, int32> PerMesh;
		TInlineComponentArray<UInstancedStaticMeshComponent*> Isms;
		W->GetComponents(Isms);
		for (const UInstancedStaticMeshComponent* C : Isms)
		{
			if (!C || !C->IsRegistered() || !C->GetStaticMesh() || !ProbeShown(C)) continue;
			const FBox Local = C->GetStaticMesh()->GetBoundingBox();
			if (!Local.IsValid) continue;
			const UMaterialInterface* M = C->GetNumMaterials() > 0 ? C->GetMaterial(0) : nullptr;
			for (int32 K = 0; K < C->GetInstanceCount(); ++K)
			{
				FTransform T;
				if (!C->GetInstanceTransform(K, T, true)) continue;
				const FBox Box = Local.TransformBy(T);
				const FVector Lo = S47Three::ToThree(Box.Min), Hi = S47Three::ToThree(Box.Max);
				if (!Overlaps(Lo, Hi)) continue;
				++PerMesh.FindOrAdd(C->GetStaticMesh()->GetName() + TEXT(" @ ") + C->GetName());
				Rows.Add({ (Hi - Lo).Size(), FString::Printf(TEXT("instance %s #%d mesh=%s mat0=%s size=[%.2f %.2f %.2f] min=[%.2f %.2f %.2f] max=[%.2f %.2f %.2f] centre_inside_fence=%d"),
					*C->GetName(), K, *C->GetStaticMesh()->GetName(), M ? *M->GetName() : TEXT("-"),
					Hi.X - Lo.X, Hi.Y - Lo.Y, Hi.Z - Lo.Z, Lo.X, Lo.Y, Lo.Z, Hi.X, Hi.Y, Hi.Z, Inside((Lo + Hi) * .5) ? 1 : 0) });
			}
		}
		Rows.Sort([](const FRow& L, const FRow& R) { return L.Size < R.Size; });
		UE_LOG(LogTemp, Display, TEXT("S47 pool probe: %d visible meshes/instances with bounds in the pool area (fence widened 3 m); smallest first, up to 120 lines. Flora instances regenerate with camera and quality, so indices are not stable identities."), Rows.Num());
		for (const auto& Pair : PerMesh) UE_LOG(LogTemp, Display, TEXT("S47 pool probe instances per component: %s = %d"), *Pair.Key, Pair.Value);
		for (int32 I = 0; I < FMath::Min(120, Rows.Num()); ++I) UE_LOG(LogTemp, Display, TEXT("S47 pool probe row %d diag=%.2f %s"), I, Rows[I].Size, *Rows[I].Text);
	}
}

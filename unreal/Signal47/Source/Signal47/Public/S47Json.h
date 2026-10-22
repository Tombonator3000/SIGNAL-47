#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// The saves are the web version's JSON (story/state.ts, each chapter's serialize()): the same
// field names and shapes, so a ported chapter writes what its TypeScript wrote. These keep the
// reading and writing short:
//
//   S.stage = J::Str(O, "stage", "collect-camera");     J::Set(O, "stage", S.stage);
//   S.dev1 = J::Num(O, "dev1");                         J::Set(O, "dev1", S.dev1);
//   S.mark1 = J::Uv(O, "mark1");  (an optional [u, v])  J::SetUv(O, "mark1", S.mark1);
namespace S47J
{
	using FObj = TSharedPtr<FJsonObject>;

	inline FObj New() { return MakeShared<FJsonObject>(); }
	inline bool Has(const FObj& O, const TCHAR* K) { return O.IsValid() && O->HasField(K) && !O->GetField<EJson::None>(K)->IsNull(); }
	inline double Num(const FObj& O, const TCHAR* K, double D = 0) { double V = D; if (O.IsValid()) O->TryGetNumberField(K, V); return V; }
	inline int32 Int(const FObj& O, const TCHAR* K, int32 D = 0) { return int32(Num(O, K, D)); }
	inline bool Bool(const FObj& O, const TCHAR* K, bool D = false) { bool V = D; if (O.IsValid()) O->TryGetBoolField(K, V); return V; }
	inline FString Str(const FObj& O, const TCHAR* K, const FString& D = FString()) { FString V = D; if (O.IsValid() && !O->TryGetStringField(K, V)) V = D; return V; }
	inline FObj Obj(const FObj& O, const TCHAR* K) { const FObj* V = nullptr; return O.IsValid() && O->TryGetObjectField(K, V) ? *V : FObj(); }
	inline TArray<TSharedPtr<FJsonValue>> Arr(const FObj& O, const TCHAR* K)
	{
		const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
		return O.IsValid() && O->TryGetArrayField(K, A) ? *A : TArray<TSharedPtr<FJsonValue>>();
	}
	inline TArray<FString> Strs(const FObj& O, const TCHAR* K)
	{
		TArray<FString> Out;
		for (const TSharedPtr<FJsonValue>& V : Arr(O, K)) { FString S; if (V->TryGetString(S)) Out.Add(S); }
		return Out;
	}
	inline TArray<double> Nums(const FObj& O, const TCHAR* K)
	{
		TArray<double> Out;
		for (const TSharedPtr<FJsonValue>& V : Arr(O, K)) { double D = 0; if (V->TryGetNumber(D)) Out.Add(D); }
		return Out;
	}
	// an optional [u, v] (a mark on a print)
	inline TOptional<FVector2D> Uv(const FObj& O, const TCHAR* K)
	{
		const TArray<double> A = Nums(O, K);
		return A.Num() >= 2 ? TOptional<FVector2D>(FVector2D(A[0], A[1])) : TOptional<FVector2D>();
	}

	inline void Set(const FObj& O, const TCHAR* K, double V) { O->SetNumberField(K, V); }
	inline void Set(const FObj& O, const TCHAR* K, int32 V) { O->SetNumberField(K, V); }
	inline void Set(const FObj& O, const TCHAR* K, bool V) { O->SetBoolField(K, V); }
	inline void Set(const FObj& O, const TCHAR* K, const FString& V) { O->SetStringField(K, V); }
	inline void Set(const FObj& O, const TCHAR* K, const TCHAR* V) { O->SetStringField(K, V); }
	inline void Set(const FObj& O, const TCHAR* K, const FObj& V) { if (V.IsValid()) O->SetObjectField(K, V); else O->SetField(K, MakeShared<FJsonValueNull>()); }
	inline void SetNull(const FObj& O, const TCHAR* K) { O->SetField(K, MakeShared<FJsonValueNull>()); }
	inline void Set(const FObj& O, const TCHAR* K, const TArray<FString>& V)
	{
		TArray<TSharedPtr<FJsonValue>> A;
		for (const FString& S : V) A.Add(MakeShared<FJsonValueString>(S));
		O->SetArrayField(K, A);
	}
	inline void Set(const FObj& O, const TCHAR* K, const TArray<double>& V)
	{
		TArray<TSharedPtr<FJsonValue>> A;
		for (double D : V) A.Add(MakeShared<FJsonValueNumber>(D));
		O->SetArrayField(K, A);
	}
	inline void SetUv(const FObj& O, const TCHAR* K, const TOptional<FVector2D>& V)
	{
		if (V) Set(O, K, TArray<double>{ V->X, V->Y });
	}
	// a field that is a string or null (three-way state: 'passive' | 'active' | null)
	inline void SetOrNull(const FObj& O, const TCHAR* K, const FString& V) { if (V.IsEmpty()) SetNull(O, K); else Set(O, K, V); }

	inline FString Write(const FObj& O, bool bPretty = false)
	{
		FString Out;
		if (bPretty) { TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Out); FJsonSerializer::Serialize(O.ToSharedRef(), W); }
		else { TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> W = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out); FJsonSerializer::Serialize(O.ToSharedRef(), W); }
		return Out;
	}
	inline FObj Read(const FString& Text)
	{
		FObj O;
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), O);
		return O;
	}
	inline FObj Clone(const FObj& O) { return O.IsValid() ? Read(Write(O)) : FObj(); }
}

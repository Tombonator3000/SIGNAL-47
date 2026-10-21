#include "S47Image.h"

#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "TextureResource.h"

namespace
{
	TMap<uint32, TWeakObjectPtr<UTexture2D>> UrlCache;
	TMap<FString, TWeakObjectPtr<UTexture2D>> ArtCache;
	TArray<TStrongObjectPtr<UTexture2D>> Keep; // pictures stay for the session (a few dozen)

	UTexture2D* Decode(const TArray<uint8>& Bytes)
	{
		IImageWrapperModule& M = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		const EImageFormat F = M.DetectImageFormat(Bytes.GetData(), Bytes.Num());
		if (F == EImageFormat::Invalid) return nullptr;
		TSharedPtr<IImageWrapper> W = M.CreateImageWrapper(F);
		TArray<uint8> Raw;
		if (!W.IsValid() || !W->SetCompressed(Bytes.GetData(), Bytes.Num()) || !W->GetRaw(ERGBFormat::BGRA, 8, Raw)) return nullptr;
		const int32 Wd = W->GetWidth(), Ht = W->GetHeight();
		TArray<FColor> Px;
		Px.SetNumUninitialized(Wd * Ht);
		FMemory::Memcpy(Px.GetData(), Raw.GetData(), Px.Num() * sizeof(FColor));
		return S47Image::FromPixels(Px, Wd, Ht);
	}
}

UTexture2D* S47Image::FromPixels(const TArray<FColor>& Pixels, int32 W, int32 H)
{
	if (W <= 0 || H <= 0 || Pixels.Num() < W * H) return nullptr;
	UTexture2D* T = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
	if (!T) return nullptr;
	T->SRGB = true;
	void* Data = T->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, Pixels.GetData(), W * H * sizeof(FColor));
	T->GetPlatformData()->Mips[0].BulkData.Unlock();
	T->UpdateResource();
	Keep.Emplace(T);
	return T;
}

UTexture2D* S47Image::FromDataUrl(const FString& Url)
{
	const int32 Comma = Url.Find(TEXT(","));
	if (!Url.StartsWith(TEXT("data:image/")) || Comma < 0) return nullptr;
	const uint32 H = GetTypeHash(Url);
	if (const TWeakObjectPtr<UTexture2D>* Have = UrlCache.Find(H); Have && Have->IsValid()) return Have->Get();
	TArray<uint8> Bytes;
	if (!FBase64::Decode(Url.Mid(Comma + 1), Bytes)) return nullptr;
	UTexture2D* T = Decode(Bytes);
	if (T) UrlCache.Add(H, T);
	return T;
}

FString S47Image::ToDataUrl(const TArray<FColor>& Pixels, int32 W, int32 H, bool bJpeg, int32 Quality)
{
	IImageWrapperModule& M = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> Wr = M.CreateImageWrapper(bJpeg ? EImageFormat::JPEG : EImageFormat::PNG);
	if (!Wr.IsValid() || !Wr->SetRaw(Pixels.GetData(), W * H * sizeof(FColor), W, H, ERGBFormat::BGRA, 8)) return FString();
	const TArray64<uint8> Out = Wr->GetCompressed(bJpeg ? Quality : 0);
	return FString(bJpeg ? TEXT("data:image/jpeg;base64,") : TEXT("data:image/png;base64,")) + FBase64::Encode(Out.GetData(), int32(Out.Num()));
}

bool S47Image::ReadPixels(UTextureRenderTarget2D* RT, TArray<FColor>& Out)
{
	if (!RT) return false;
	FTextureRenderTargetResource* R = RT->GameThread_GetRenderTargetResource();
	return R && R->ReadPixels(Out);
}

UTexture2D* S47Image::Art(const FString& Name)
{
	if (const TWeakObjectPtr<UTexture2D>* Have = ArtCache.Find(Name); Have && Have->IsValid()) return Have->Get();
	for (const TCHAR* Ext : { TEXT(".png"), TEXT(".jpg") })
	{
		TArray<uint8> Bytes;
		if (FFileHelper::LoadFileToArray(Bytes, *(FPaths::ProjectContentDir() / TEXT("WebData/art") / (Name + Ext))))
		{
			UTexture2D* T = Decode(Bytes);
			if (T) { ArtCache.Add(Name, T); return T; }
		}
	}
	return nullptr;
}

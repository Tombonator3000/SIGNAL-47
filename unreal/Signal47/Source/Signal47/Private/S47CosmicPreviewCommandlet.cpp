#include "S47CosmicPreviewCommandlet.h"
#include "S47CosmicAmbient.h"
#include "Misc/Parse.h"

US47CosmicPreviewCommandlet::US47CosmicPreviewCommandlet()
{
    IsClient = false; IsServer = false; IsEditor = true;
    LogToConsole = true; ShowErrorCount = true;
}

int32 US47CosmicPreviewCommandlet::Main(const FString& Params)
{
    FString Path;
    if (!FParse::Value(*Params, TEXT("S47CosmicPreview="), Path) || Path.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Expected -run=S47CosmicPreview -S47CosmicPreview=/absolute/path/preview.wav"));
        return 1;
    }
    return FS47CosmicAmbient::RenderPreview(Path) ? 0 : 1;
}

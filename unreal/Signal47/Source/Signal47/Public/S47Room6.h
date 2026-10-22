#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "Tickable.h"
#include "UObject/Object.h"
#include "S47Room6.generated.h"

class US47Main;
class UTexture2D;

// world/Room6.ts, the behaviour: room 6 at Sierra Motor Court, where N. Vega waits (chapter four).
// The geometry is the web version's own (Tools/webexport, world.room6.*); this holds its parts by
// their paths and does what the class's methods do: the table lamp (its flood in the room's set and
// its shade), the television (snow on every channel, 96x72 at 15 Hz, lighting the room blue), the
// neon through the curtains, the papers on the table, Nora's head, the proxies and the anchors.
//
// World.update steps the room while the player is in it (every frame, also behind a panel). The
// world's step is not in this version yet, so the room ticks itself on the same rule.
UCLASS()
class SIGNAL47_API US47Room6 : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	void Init(US47Main* InM);
	virtual void BeginDestroy() override;

	// proxies: door, window, bathroom, bed, phone, tv, photo, table, lamp, fieldCard, letter,
	// correction, walkie, nora; objs: door, bed, nightstand, phone, dresser, tv, photo, table, lamp,
	// fieldCard, letter, correction, shoebox, walkie, nora, noraHead
	FS47Obj Proxy(const FString& Id) const;
	FS47Obj Obj(const FString& Id) const;
	/** anchors: arrive, talk, exit ({x, z, yaw} in the world) */
	struct FAnchor { double X = 0, Z = 0, Yaw = 0; };
	FAnchor Anchor(const FString& Id) const;

	bool bLampOn = true;
	void SetLamp(bool bOn);
	// the television: snow on every channel, lighting the room blue (Tom, 4 October)
	bool bTvOn = false;
	/** Switch the television on (snow, a flickering blue light on the room) or off. */
	void SetTv(bool bOn);
	void Update(double Dt, double T);

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bInit; }
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(US47Room6, STATGROUP_Tickables); }
	virtual ETickableTickType GetTickableTickType() const override { return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Conditional; }

	// the snow as it is drawn (the self test)
	const TArray<FColor>& Snow() const { return TvPx; }
	static constexpr int32 TV_W = 96, TV_H = 72;
	/** One field of snow at t into Px (96x72, top row first), as update() draws it. */
	static void DrawSnow(TArray<FColor>& Px, double T, TFunctionRef<double()> Random);

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	UPROPERTY() TObjectPtr<UTexture2D> TvTex;
	bool bInit = false;
	// the room's flood set ('motel', six slots) and its slots: the table lamp, the neon, the television
	int32 Flood = -1, LampSlot = 0, NeonSlot = 2, TvSlot = 4;
	double TvAcc = 0, TvGlow = 0;
	TArray<FColor> TvPx;
	void UploadSnow();
};

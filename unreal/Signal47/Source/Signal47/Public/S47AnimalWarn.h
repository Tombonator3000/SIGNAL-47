#pragma once
#include "CoreMinimal.h"
class US47Main;

// ANIMALS-20261108 / N1110 (Tom, 8 November: "birds that take off all at once, a coyote that stands and stares at you before an
// event"): the animals as a warning, a few seconds before some of the night's events, and only now and then.
//
// FS47Happenings::StartEvent asks Hold() for the events the animals warn for (Warned): out on foot at night, the animals first, if
// this night's roll allows it (the warning scripts' own gates: Content/Director/Events/animals_warn_birds.json and
// animals_warn_coyote.json, their hours and odds, the same each time that night is asked). A covey goes up out of the brush at
// once, or a coyote stops 18 to 30 m off and stares at the player (FS47Animals::Warn); the script says what is thought and kept.
// Then the event is held for Lead seconds and started as it would have been (Step). At most one warning a night, never in the
// first shift, never indoors or at the wheel, and the event's own tier and hours are not touched: it is the same event, a moment
// later. The animals are the ones the game already has (S47Animals); nothing new is saved: a warning not yet followed by its event
// when a game is loaded is simply asked again.
//
// (The attention system, N1110-1, is not part of this file: when it is in, its score can scale the scripts' odds in Hold.)
class SIGNAL47_API FS47AnimalWarn
{
public:
	explicit FS47AnimalWarn(US47Main* Main) : M(Main) {}

	static constexpr double Lead = 7.0;
	enum class EKind : uint8 { None, Birds, Coyote };

	/** The events the animals warn for: the ones out in the open, a figure, lights, a landing. */
	static bool Warned(const FString& Id);
	/** The script of a kind (animals_warn_birds, animals_warn_coyote). */
	static const TCHAR* ScriptOf(EKind Kind);
	/** Which animal comes first on a night (pure): seeded and the same each time it is asked. */
	static EKind First(int32 Seed, int32 Night, const FString& Id);
	/** The other one. */
	static EKind Other(EKind Kind) { return Kind == EKind::Birds ? EKind::Coyote : EKind::Birds; }

	/** StartEvent's question, before the event's own code. True: go on. False with Why: the animals first; Due() hands the event
	 *  back once Lead seconds are up. */
	bool Hold(const FString& Id, const FString& Variant, bool bOnFoot, FString& Why);
	/** An event whose lead is up, to be started now. */
	bool Due(FString& OutId, FString& OutVariant);
	/** A new game, another area: nothing is waiting. */
	void Reset();

	int32 Warnings() const { return Count; }
	bool Waiting() const { return Pending.Num() > 0; }

	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	struct FWait { FString Id, Variant; double At = 0; };
	TArray<FWait> Pending;
	int32 Night = 0, Count = 0;
	bool bUsed = false;
	TSet<FString> Asked;  // the events asked about this night (once each)
};

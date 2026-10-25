#include "S47EveningPack.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

bool FS47EveningPack::SelfTest(FString& Error)
{
	FS47EveningPack P;
	if (!P.Initialize(Error)) return false;
#define EVENING_CHECK(Condition, Message) do { if (!(Condition)) { Error = TEXT(Message); return false; } } while (false)
	const auto Drain = [&P]() { while (P.CurrentLine()) P.Advance(); };
	const auto Run = [&P, &Drain](const TCHAR* Id)
	{
		const auto* Scene = P.FindScene(Id);
		if (!Scene || !P.SetPhase(Scene->Phase) || !P.Start(Id)) return false;
		Drain(); return true;
	};
	const auto Choose = [&P, &Drain](const TCHAR* Id)
	{
		if (!P.Select(Id)) return false;
		Drain(); return true;
	};
	const auto Handover = [&P, &Run, &Choose]()
	{
		return Run(TEXT("I11_SARO_ARRIVAL")) && Run(TEXT("D01_HANDOVER")) &&
			Choose(P.Flag(TEXT("food_bought")) ? TEXT("D01_CH_FOOD") : TEXT("D01_CH_NOFOOD")) &&
			Choose(P.Flag(TEXT("food_bought")) ? TEXT("D01_CH_FOOD_BRIEF") : TEXT("D01_CH_NOFOOD_BRIEF"));
	};
	const auto RoundTrip = [&P, &Error]()
	{
		FString Serialized;
		FJsonSerializer::Serialize(P.Save().ToSharedRef(), TJsonWriterFactory<>::Create(&Serialized));
		TSharedPtr<FJsonObject> Snapshot;
		return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Serialized), Snapshot) && P.Load(Snapshot, Error);
	};
	EVENING_CHECK(P.Scenes.Num() == 101 && P.DocumentData.Num() == 17, "Evening source scene/document count");
	int32 MeasuredCues = 0;
	for (const auto& Scene : P.Scenes) for (const auto& Line : Scene.Value.Lines) if (Line.bRecord)
	{
		EVENING_CHECK(Line.Duration > 0 && Line.SpeechStart >= 0 && Line.SpeechEnd > Line.SpeechStart && Line.SpeechEnd <= Line.Duration, "Every voiced cue carries measured speech timing");
		++MeasuredCues;
	}
	EVENING_CHECK(MeasuredCues == 110, "109 measured recordings cover 110 voiced cues");
	const auto& Original = P.FindScene(TEXT("D01_HANDOVER"))->Lines[0];
	const auto& Fragment = P.FindScene(TEXT("D61_FIXED_FRAGMENT"))->Lines[0];
	EVENING_CHECK(Original.Duration == Fragment.Duration && Original.SpeechStart == Fragment.SpeechStart && Original.SpeechEnd == Fragment.SpeechEnd, "Fixed fragment inherits exact original speech window");
	EVENING_CHECK(!P.CanStart(TEXT("D01_HANDOVER")) && !P.CanStart(TEXT("I03_MENU")), "Phase and choice scenes are gated");
	EVENING_CHECK(!P.SetExternal(TEXT("food_bought"), true) && !P.SetExternal(TEXT("ctx_fake"), true), "Only declared world flags are externally writable");

	// Direct route. Reading a menu does not finish the handover or grant its papers.
	EVENING_CHECK(Run(TEXT("I01_DRIVE")), "Direct route: cheerful radio");
	EVENING_CHECK(P.Flag(TEXT("arrival_radio_heard")) && !P.Flag(TEXT("intro_arrived")), "Radio cannot park the car");
	EVENING_CHECK(Run(TEXT("I11_SARO_ARRIVAL")) && P.Flag(TEXT("intro_arrived")), "Direct route: parking arrival");
	EVENING_CHECK(!P.CanStart(TEXT("I11_SARO_ARRIVAL")), "Arrival cannot repeat");
	EVENING_CHECK(Run(TEXT("D01_HANDOVER")), "Direct route: phone handover");
	EVENING_CHECK(P.CurrentScene() && P.CurrentScene()->Channel == TEXT("phone") && !P.Flag(TEXT("handover_done")) && P.Documents().IsEmpty(), "Phone menu is not a completed handover");
	EVENING_CHECK(P.Choices().Num() == 1 && P.Choices()[0]->Id == TEXT("D01_CH_NOFOOD"), "Direct route: only honest food response");
	EVENING_CHECK(Choose(TEXT("D01_CH_NOFOOD")) && Choose(TEXT("D01_CH_NOFOOD_BRIEF")), "Direct route: finish phone");
	EVENING_CHECK(P.Flag(TEXT("handover_done")) && P.Flag(TEXT("intro_completed")) && !P.Flag(TEXT("food_bought")) && P.Documents().Num() == 3, "Direct route: completion effects");
	EVENING_CHECK(RoundTrip() && !P.CanStart(TEXT("D01_HANDOVER")), "Completed phone stays completed after save/load");
	EVENING_CHECK(Run(TEXT("N01_ROUTINE")) && P.Documents().Num() == 3, "Normal references deduplicate stable document IDs");
	EVENING_CHECK(P.SetPhase(TEXT("signal")) && !P.CanStart(TEXT("N01_COMPARE")), "Comparison requires actual residual");
	P.SetExternal(TEXT("ctx_signal_present"), true);
	EVENING_CHECK(Run(TEXT("N01_COMPARE")) && Choose(TEXT("N01_CH_COMPARE_CAUSE")), "Wrong cause provides correction");
	EVENING_CHECK(!P.Flag(TEXT("record_compared")) && Choose(TEXT("N01_CH_REVISE")) && P.Flag(TEXT("record_compared")) && P.Documents().Num() == 3, "Unresolved correction retains reference papers");

	// Burger purchase and pending reply must survive one serialized snapshot.
	P.Reset();
	EVENING_CHECK(Run(TEXT("I02_DINER_HELLO")) && Choose(TEXT("I02_MENU")), "Burger route: open menu");
	EVENING_CHECK(P.Select(TEXT("I03_BURGER")), "Burger route: purchase");
	EVENING_CHECK(P.Flag(TEXT("food_bought")) && P.Flag(TEXT("arrival_order_burger")) && !P.Flag(TEXT("arrival_order_sandwich")), "Burger route: one order");
	EVENING_CHECK(P.GetPendingChoice() == TEXT("I03_BURGER") && P.CurrentScene()->Id == TEXT("I04_BURGER") && P.NextLine() == 0, "Purchase atomically records its pending response");
	EVENING_CHECK(RoundTrip() && P.CurrentLine()->Id == TEXT("I04_BURGER_001") && P.GetPendingChoice() == TEXT("I03_BURGER"), "Pending serving response restored exactly");
	const auto MissingPayment = P.Save(); MissingPayment->GetObjectField(TEXT("flags"))->SetBoolField(TEXT("food_bought"), false);
	EVENING_CHECK(!P.Load(MissingPayment, Error) && P.Flag(TEXT("food_bought")) && P.CurrentScene()->Id == TEXT("I04_BURGER"), "Pending response cannot load without its purchase");
	EVENING_CHECK(!P.Select(TEXT("I03_BURGER")) && !P.Start(TEXT("I02_DINER_HELLO")), "Cannot buy twice while response is pending");
	P.Advance();
	EVENING_CHECK(P.GetPendingChoice().IsEmpty() && P.NextLine() == 1 && RoundTrip() && P.NextLine() == 1, "Partly read response resumes at next line");
	Drain();
	EVENING_CHECK(Choose(TEXT("I04_TAKE")) && P.Flag(TEXT("food_taken")) && !P.Flag(TEXT("food_eaten")), "Burger takeaway is a single portion");
	EVENING_CHECK(Handover() && P.Flag(TEXT("handover_done")), "Burger route reaches food phone branch");
	P.SetPhase(TEXT("all_night"));
	EVENING_CHECK(!P.CanStart(TEXT("I12_ALL_NIGHT_BOUGHT")), "Late diner requires actual all-night chapter");
	P.SetExternal(TEXT("ctx_all_night_active"), true);
	EVENING_CHECK(Run(TEXT("I12_ALL_NIGHT_BOUGHT")) && !P.CanStart(TEXT("I13_ALL_NIGHT_NO_FOOD")) && !P.CanStart(TEXT("I12_ALL_NIGHT_BOUGHT")), "Purchased late diner greeting plays once");

	P.Reset();
	EVENING_CHECK(Run(TEXT("I02_DINER_HELLO")) && Choose(TEXT("I02_MENU")) && Choose(TEXT("I03_SANDWICH")) && Choose(TEXT("I05_EAT")), "Sandwich dine-in route");
	EVENING_CHECK(P.Flag(TEXT("arrival_order_sandwich")) && P.Flag(TEXT("food_eaten")) && !P.Flag(TEXT("food_taken")) && Handover(), "Sandwich dine-in reaches SARO");
	P.Reset();
	EVENING_CHECK(Run(TEXT("I02_DINER_HELLO")) && Choose(TEXT("I02_WATER")) && Choose(TEXT("I08_LEAVE")) && Handover(), "Water-only stop remains valid without food");
	EVENING_CHECK(!P.Flag(TEXT("food_bought")) && P.Flag(TEXT("handover_done")), "Water-only does not fabricate a meal");
	P.SetPhase(TEXT("all_night")); P.SetExternal(TEXT("ctx_all_night_active"), true);
	EVENING_CHECK(!P.CanStart(TEXT("I12_ALL_NIGHT_BOUGHT")) && Run(TEXT("I13_ALL_NIGHT_NO_FOOD")), "No-purchase late diner greeting");

	// Office main route is independent of video, and service success requires host evidence.
	P.Reset();
	EVENING_CHECK(Handover() && P.SetPhase(TEXT("archive")) && !P.CanStart(TEXT("D02_SERVICE_DRAWER")), "Office requires actual archive gate");
	P.SetExternal(TEXT("ctx_archive_open"), true);
	EVENING_CHECK(!P.CanStart(TEXT("D02_OPEN_OFFICE")) && Run(TEXT("D02_SERVICE_DRAWER")) && Run(TEXT("D02_OPEN_OFFICE")) && Run(TEXT("D02_TAKE_FIELD_FOLDER")), "Office spare key, door, and short field-folder route");
	EVENING_CHECK(P.Flag(TEXT("office_evidence_taken")) && !P.Flag(TEXT("vhs_seen")) && P.Documents().Contains(TEXT("EV_DALE_INDEX")) && P.Documents().Contains(TEXT("EV_DALE_NOTE")), "Main field lead never requires VHS");
	EVENING_CHECK(!P.CanStart(TEXT("D02_START_FIELD_TAPE")), "VHS play requires actual power");
	P.SetExternal(TEXT("ctx_power_available"), true);
	EVENING_CHECK(Run(TEXT("D02_START_FIELD_TAPE")) && !P.Flag(TEXT("vhs_seen")), "Starting VHS does not mean watched");
	EVENING_CHECK(RoundTrip() && P.Flag(TEXT("office_tape_started")) && !P.Flag(TEXT("vhs_seen")) && !P.Flag(TEXT("ctx_power_available")), "Interrupted tape retains started state; host must restore real power");
	EVENING_CHECK(Run(TEXT("T01_SERVICE_LOG")) && Run(TEXT("T01_REQUEST_CHECK")) && !P.CanStart(TEXT("T01_NORMAL_RESULT")) && !P.Flag(TEXT("trafo_normal_checked")), "Requesting service cannot pass the normal test");
	P.SetExternal(TEXT("ctx_trafo_service_ok"), true);
	EVENING_CHECK(Run(TEXT("T01_NORMAL_RESULT")) && P.Flag(TEXT("trafo_normal_checked")), "Actual service result enables normal inspection card");
	EVENING_CHECK(Run(TEXT("T02_REFERENCE")) && Run(TEXT("T02_CHECKS")), "T02 requires field reference and normal check");
	EVENING_CHECK(P.Choices().Num() == 1 && !P.Select(TEXT("T02_CH_VERIFIED")) && Choose(TEXT("T02_CH_UNRESOLVED")) && !P.Flag(TEXT("trafo_anomaly_checked")), "Partial T02 stays unresolved without blocking the story");
	P.SetExternal(TEXT("ctx_trafo_isolation_verified"), true);
	P.SetExternal(TEXT("ctx_trafo_backup_excluded"), true);
	P.SetExternal(TEXT("ctx_trafo_indicator_checked"), true);
	EVENING_CHECK(Run(TEXT("T02_CHECKS")) && P.Choices().Num() == 1, "Three controls cannot confirm T02");
	P.SetExternal(TEXT("ctx_trafo_lamp_persists"), true);
	EVENING_CHECK(P.Choices().Num() == 1, "Four controls without the actual reserved photo cannot confirm T02");
	P.SetExternal(TEXT("ctx_trafo_photo_available"), true);
	EVENING_CHECK(P.Choices().Num() == 2, "Four external controls and a real photo expose verified choice");
	P.SetExternal(TEXT("ctx_trafo_indicator_checked"), false);
	EVENING_CHECK(!P.Select(TEXT("T02_CH_VERIFIED")), "Selection rechecks changed external results");
	P.SetExternal(TEXT("ctx_trafo_indicator_checked"), true);
	EVENING_CHECK(P.Select(TEXT("T02_CH_VERIFIED")) && RoundTrip(), "A pending verified response round-trips before its host rebuilds context");
	P.ValidateWorldRestore(true, true, true, true, false);
	EVENING_CHECK(!P.CurrentScene() && !P.Flag(TEXT("trafo_anomaly_checked")), "Missing current physical/photo proof cancels a restored verified response");
	for (const TCHAR* K : { TEXT("ctx_trafo_isolation_verified"), TEXT("ctx_trafo_backup_excluded"), TEXT("ctx_trafo_indicator_checked"), TEXT("ctx_trafo_lamp_persists"), TEXT("ctx_trafo_photo_available") }) P.SetExternal(K, true);
	EVENING_CHECK(Run(TEXT("T02_CHECKS")), "A cancelled response remains retryable after fresh controls");
	EVENING_CHECK(Choose(TEXT("T02_CH_VERIFIED")) && P.Flag(TEXT("trafo_anomaly_checked")) && !P.Flag(TEXT("dale_field_call_done")), "Player must report result for Dale to know");
	P.SetExternal(TEXT("ctx_room_quiet"), true);
	EVENING_CHECK(P.Start(TEXT("D61_FIXED_FRAGMENT")), "Fixed Dale fragment after verified field observation");
	EVENING_CHECK(P.CurrentScene()->Mode == TEXT("fixed") && P.CurrentScene()->Channel == TEXT("radio") && P.CurrentScene()->Choices.IsEmpty() && P.CurrentLine()->RecordingId == TEXT("D01_HANDOVER_001"), "Fixed fragment reuses exact original cue without answer choices");
	Drain();
	EVENING_CHECK(Run(TEXT("D41_FIELD_WALKIE")) && P.Flag(TEXT("dale_field_call_done")), "Field report uses walkie");

 // A private-folder menu is not an implicit read of the whole collection.
 P.Reset();
 P.SetExternal(TEXT("ctx_archive_open"), true);
 EVENING_CHECK(Run(TEXT("D02_SERVICE_DRAWER")) && Run(TEXT("D02_OPEN_OFFICE")) && Run(TEXT("D02_TAKE_FIELD_FOLDER")), "Short office route remains independent of private documents");
 EVENING_CHECK(!P.Flag(TEXT("office_skylog_read")) && !P.Documents().Contains(TEXT("EV_DALE_SKYLOG")), "Taking field folder does not read private notes");
 EVENING_CHECK(Run(TEXT("D02_READ_SKYLOG")) && P.Flag(TEXT("office_skylog_read")) && !P.Flag(TEXT("office_witness_read")) && !P.Flag(TEXT("vhs_seen")), "One actual document acknowledgement grants only that page");
 EVENING_CHECK(RoundTrip() && P.Flag(TEXT("office_skylog_read")) && !P.Flag(TEXT("office_witness_read")), "Optional read states survive exact serialization independently");
 P.ValidateOfficeReadDocuments({});
 EVENING_CHECK(!P.Flag(TEXT("office_skylog_read")) && !P.Documents().Contains(TEXT("EV_DALE_SKYLOG")) && P.Documents().Contains(TEXT("EV_DALE_INDEX")), "Missing physical read history removes optional credits without removing field sheet");
 P.ValidateOfficeReadDocuments({TEXT("EV_DALE_SKYLOG"),TEXT("EV_DALE_ARRAY_SERVICE")});
 EVENING_CHECK(P.Flag(TEXT("office_skylog_read")) && P.Flag(TEXT("office_array_service_read")) && !P.Flag(TEXT("vhs_seen")) && !P.Flag(TEXT("trafo_anomaly_checked")), "Restored service appendix is not completed film or field experiment");

	// Adversarial checks protect invariants even when data gains new content later.
	P.Reset();
	P.Scenes.FindChecked(TEXT("I02_DINER_HELLO")).OnComplete.Flags.Add(TEXT("handover_done"), true);
	EVENING_CHECK(Run(TEXT("I02_DINER_HELLO")) && !P.Flag(TEXT("handover_done")), "Menu on_complete never executes before a leaf");
	EVENING_CHECK(Choose(TEXT("I02_MENU")), "Adversarial menu route");
	P.Scenes.FindChecked(TEXT("I04_BURGER")).Requires.All.Add(TEXT("office_open"));
	EVENING_CHECK(!P.Select(TEXT("I03_BURGER")) && !P.Flag(TEXT("food_bought")), "Target requirements checked before committing a purchase");
	P.Scenes.FindChecked(TEXT("I04_BURGER")).Requires.All.Remove(TEXT("office_open"));
	P.Scenes.FindChecked(TEXT("I03_MENU")).Choices[0].OnSelect.Flags.Add(TEXT("ctx_signal_present"), true);
	EVENING_CHECK(!P.Select(TEXT("I03_BURGER")) && !P.Flag(TEXT("ctx_signal_present")), "Scripts cannot write external context via choices");
	P.Reset();
	P.Scenes.FindChecked(TEXT("I11_SARO_ARRIVAL")).OnComplete.Flags.Add(TEXT("ctx_signal_present"), true);
	EVENING_CHECK(Run(TEXT("I11_SARO_ARRIVAL")) && !P.Flag(TEXT("ctx_signal_present")), "Leaf effects cannot write external context");
	const auto Valid = P.Save();
	Valid->SetNumberField(TEXT("version"), 999);
	EVENING_CHECK(!P.Load(Valid, Error) && P.Flag(TEXT("intro_arrived")), "Unknown save version rejected atomically");
	const auto BadCursor = P.Save(); BadCursor->SetNumberField(TEXT("next_line"), 0.5);
	EVENING_CHECK(!P.Load(BadCursor, Error) && P.Flag(TEXT("intro_arrived")), "Fractional cursor rejected atomically");
	const auto BadFlags = P.Save(); BadFlags->GetObjectField(TEXT("flags"))->SetBoolField(TEXT("ctx_signal_present"), true);
	EVENING_CHECK(!P.Load(BadFlags, Error) && !P.Flag(TEXT("ctx_signal_present")), "Save cannot smuggle external flags into scripts");
	const auto BadDoc = P.Save(); BadDoc->SetArrayField(TEXT("documents"), {MakeShared<FJsonValueString>(TEXT("UNKNOWN_DOCUMENT"))});
	EVENING_CHECK(!P.Load(BadDoc, Error), "Unknown document IDs rejected");
	Error.Reset();
#undef EVENING_CHECK
	return true;
}

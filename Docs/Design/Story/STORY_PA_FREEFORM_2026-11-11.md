# Story på Freeform-grunnlag: kartlegging og plan

**Status:** Under arbeid, steg 1–4 gjort (steg 4 ikke bygget ennå). **Gjelder:** STORY-ON-FREEFORM-01 ([TODO](../../Active/TODO.md), [DECISIONS](../../Active/DECISIONS.md) 11. november). **Sist kontrollert:** 11. november 2026, på `cbe78986`.

Retning: alt i Freeform går inn i Story, aldri motsatt. Freeform endres ikke for Storys skyld; Freeform-koden får bare tillegg som er av uten Story-regi. Stier er under `unreal/Signal47/Source/Signal47/Private/` om ikke annet står; linjetall kan flytte seg.

## Funn

- **Én bryter.** `ES47SessionMode { Story, Freeform }` (`Public/S47Session.h:4`), lest via `US47Main::IsFreeform()`; rundt 470 grener. NEW STORY går via åpningskortet til `StartGame` uten `bFreeform` (`S47Flow.cpp:814-824, 351`); NEW FREEFORM via `RequestNewFreeform` (`:868-883`). Modus settes `:923-934`.
- **Ingen egne kart, men annet utseende.** Samme verden lastes. `S47FarmWorld::Enabled(M)` er bare `IsFreeform()` (`S47FarmWorld.cpp:459`) og slår av relieff og fjellkjede (`S47FreeformTerrain.cpp:488-495`), veikant/gård/åpent land (`S47World.cpp:2528`) og nye naturregler (`S47WorldLife.cpp:201`). Vær er av i Story (`S47WorldLife.cpp:377`, `S47WeatherLive.cpp:110`).
- **Freeform-systemene kjører ikke i Story.** `FS47FreeformSession` startes og tikkes bare i Freeform (`S47Flow.cpp:1053-1104`, `S47Main.cpp:117`): jobber, OPS, Director, Happenings, hendelsesskript, Attention, DoorLoop, Taken, Fatigue, Hide, WeatherLive, Mail, Dale (Predecessor), Visitors, Townsfolk, BigNight, WeekCase. Story kjører i stedet Evening, Observing, Spine, Horror, Anomaly og Substation T01/T02 (`S47Main.cpp:118-149`). Også av i Story: urolaget (Omen, GearFlicker, Soundscape, Animals), Jeepens drivstoff/last/taklampe, garasjen, lommelyktbatteri, kart-FillMap, NightLine og TV-programmet.
- **Story-innhold verdt å beholde:** plottet SC01–SC13 (`PRODUKSJON_SC01_SC13.md`), dialogpakkene `unreal/Narrative/EveningShift` og `HorrorStory` med valg og flagg (motor: `FS47EveningPack`, `Public/S47EveningPack.h:48-96`), stjernehull/meteorpassasje, Nora-merket på kartet, strømbruddet, finalen (`Flow::Finale`, `S47Flow.cpp:1252`) og 47-slettingen i tittelen. Motoren under (kapitler, Spine, Horror, Story-klokka) erstattes.
- **Freeform dekker allerede mye av skrekken:** SC02-flukten = Roswell-jakt og Hide; SC03-sheriffen = DeputyRound; SC08 = array_presence; SC11-beleiringen = attention_visit og DoorLoop; bortføring = Taken; strømbruddet = `feeder_fault`.
- **Hendelseslaget mangler fem ting for en regissert natt:** historieflagg, tvungen start fra regien, scene med dialogvalg, klokkekrav og mål/slutt. Ingenting holder tilfeldige store hendelser unna mens en Story-beat eier natta.
- **Største risiko er spoilervernet.** Freeforms `S47Script::LintText` (`S47Script.cpp:311-336`) avviser nettopp Storys kjerne. Story-tekst må ligge i eget lager med egen sjekk og aldri lekke inn i vanlig Freeform.

## Løsning

NEW STORY starter en ekte Freeform-økt (`SessionMode=Freeform`, alle grener på) med en adskilt historieregissør `FS47StoryDirector` oppå. Regissøren er en kort tidslinje av akter (kveld, natt, morgen) med krav på flagg og klokke. Nye tillegg i hendelseslaget, som bare gjør noe når regissøren finnes:

1. vilkår `story_flag` / `no_story_flag` (via `Extra` i `GatesHold`, `S47EventScript.cpp:371`; usanne uten regi);
2. beat `flag` (sett/fjern) og `start` (regiskript, eller tving en Freeform-hendelse via `Director->Force` / `Happenings::StartEvent`);
3. beat `scene`: spiller en EveningPack/HorrorPack-scene med valg på telefon, walkie eller radio;
4. beat `clock at_least`: skyver Freeform-klokka fram, aldri tilbake;
5. beat `objective` og `end` (`Flow::Finale` og fullført Story);
6. mens en tvungen beat går, svarer regien ja på `OneAtATime` (`S47Main.cpp:99`, tillegg), så Directors terninger venter;
7. eget manuslager `Content/Director/Story/*.json` med egen tekstsjekk; `LintText` for Freeform er uendret.

## Plan i sju steg

Hvert steg holder selvtest, reg, journey, inworld og evidence grønne. Gammel Story lever videre bak DEVELOPER fram til steg 7.

1. **Story-identitet uten innhold:** `FStart.bStoryNight` og lagringsfelt `storyDirector{v:1}`; NEW STORY starter Freeform med flagget; fortsett-linja viser STORY. Dette fjerner alle grafiske forskjeller med én gang. Risiko lav.
2. **Oppsett for én natt:** valgfri parameter til `FS47FreeformSession::Begin` (ingen opplæring/postkassestart, start ved SARO kl. 18). Standard gir uendret Freeform. Risiko middels. *Endret 12. november (Tom: «Story skal jo fremdeles begynne i bilen mot SARO»):* en ny Story-natt starter i bilen på riksveien sør i Caliche, på vei nordover mot SARO kl. 18 (`FS47StoryDirector::BeginDrive`, samme start som gamle Storys kjøretur, Freeforms egen `StartTrip`). Kveldsakten starter `evening_drive` (bilradioen spiller I01 som tekstlinjer, ankomst = til fots i SARO-området, så `evening_handover`) og `evening_diner` (valgfri stopp: I02 ved disken, I10 tilbake på veien). `-S47StoryAtSaro` gir den gamle starten ved SARO.
3. **Regissørens skjelett:** `FS47StoryDirector`, akter, historieflagg, eget lager og tekstsjekk; eies av `US47Main`, stoppes i `StartGame`. Risiko lav.
4. **Tilleggene i hendelseslaget** (liste over). Risiko middels; F10 og tekstsjekk må holde dem unna Freeform-mappene. *Gjort:* `story_flag`/`no_story_flag`, beatene `flag`, `start`, `clock`, `objective` (nøkkelen `"do"`) i eget lager `Content/Director/Story/beats` med `S47EventScript::LintStory`; Freeforms `Lint` avviser dem ved navn. Regissøren kjører manusene på sin egen `FS47EventBeats`, akter starter dem med `"scripts"`, og mens de går holdes Freeforms terninger (`Hold("story")`, `Eligible`, `OneAtATime`). En Freeform-hendelse startes ikke tvunget, men via `FS47Happenings::Request`. `scene` og `end` hører til steg 5.
5. **Dialog med valg:** beat `scene` kobler `FS47EveningPack` til Freeforms telefon/walkie/radio; D01_HANDOVER først. Risiko middels. *Skrevet 11. november:* `FS47StoryScene` (`Public/S47StoryScene.h`) spiller én scene om gangen: telefonscener ringer ved OPS og åpnes når spilleren tar telefonen (før Freeforms egen bruk av den), walkie og radio åpnes når ingen annen panel er oppe. Pakkens egne regler avgjør hva som følger. Når scenen er over, blir pakkens satte flagg og `scene_<id>` historieflagg. Dale høres med Freeforms egen stemme (`FS47DaleVoice`, `dale_de_design`); en replikk uten opptak er tekst. `evening_handover` legger inn ankomsten (I11) og lar D01 ringe; målet ryddes når `handover_done` er satt. **Avklart av Tom 11. november:** spilleren er Reyes i Story; Freeform foregår etter Story, når Reyes har forsvunnet. Derfor må alt i Freeform som sier at Reyes er borte (forgjengeren, skapet, Dales og Wards linjer om ham), være av eller skrevet som nåtid på en Story-natt. Det hører til steg 6.
6. **Natta som beats, én akt per PR:** kveld (D01, kalibrering, trafo via `feeder_fault`); midt (SC02 jakt/Hide, SC03 DeputyRound, SC05 residual og stjernehull, SC08 array_presence, SC10 rom 6); slutt (SC11 attention_visit, SC12 Ward, SC13 `end`). Ny `-S47StoryNightContract` per akt og kritikerrunde før installasjon. *Første utkast 11. november:* `start` kan starte Freeforms Director-hendelser på Story-natt (`StartForStory`); strømbruddet gir historieflaggene `power_fault` og `power_restored`, og pakkens `ctx_power_available`, `ctx_trafo_service_ok` og `ctx_room_quiet` settes fra Freeform. `evening_shift`: N01, klokka til 22:30, `feeder_fault`, mål til strømmen er tilbake. Gjenstår: D11 og D12 (D11_POWER_PHONE_003 sender spilleren til en koblingsstasjon ved en grusvei; Freeforms feil er ved veiskapet; ny tekst krever nytt `dale_de_design`-opptak, ellers feiler `build_pack.py`), dokumenter fra scenenes `give_documents`, og at en ringende scene stopper når grunnen er borte.
7. **Opprydding:** gamle Story-lagringer avvises med melding; legacy-start, kapittelmotor (`S47Story`, `S47Chapter2–6`, `S47Spine`, `S47Horror*`, `S47SC04/08/11`, `S47Evening*` unntatt `FS47EveningPack`, `S47Observing`, `S47Anomaly*`, Story-delen av `S47Substation`), Story-klokka, Story-grener i notatbok/kart/brett, finalebakken (node 1820/1821), Story-testene og `-S47Jump`-linjene i `Scripts/screenshots.sh` fjernes. Til sist `ES47SessionMode::Story`. Kamera og mørkerom i `S47Chapter1` brukes av Freeform og flyttes, ikke slettes; `AS47Story` beholdes som vert hvis Freeform fortsatt bruker `Story->Saro`.

## Risiko

- Gamle Story-lagringer trenger gammel motor til steg 7; ny Story lagres med egen identitet så eldre bygg ikke åpner dem feil.
- Freeforms første dag (postkasse, Ward-brev, opplæring, ukeøkonomi) passer ikke én natt; løses med valgfritt oppsett (steg 2).
- Tempo: Freeform har 75 aktive minutter per døgn og tidsnivåer; Story er tettere (18:00–05:29:47). Beats bruker `clock at_least` og tidsnivåene med vilje.
- Spoilergrensene må holde begge veier; Story-tekst bare i regien og pakkene.
- Dale-opptakene i pakkene må være `dale_de_design`; sjekk `Narrative/EveningShift/voice/provenance.json` før gjenbruk.

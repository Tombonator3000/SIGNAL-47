# Forfatterformat for kveldspakken

Dette er manus- og integrasjonsdata. Unreal leser den deterministiske C++-eksporten gjennom `FS47EveningPack`; `FS47Evening` kobler innkjøring og overlevering til verdenen. Spilltekst er engelsk, produksjonsmerknader norsk. JSON-modulene er sannhetskilden; lesemanus og stemmeliste genereres fra dem.

## Modul

`pack.json` har også `recording_aliases`, som kobler en cue-ID til en annen linjes eksisterende grunnopptak. Begge linjer må ha identisk aktør, stemmeprofil og tekst. Kanalfilter kan være forskjellig. Kilden kan ikke selv være alias. Eksportene markerer `reuse_of` og `recording_required`; D61 skal gjenbruke D01s første take.

En modul har `module_id`, `flags` (objekt), `scenes` (liste), `documents` (liste), `media` (liste) og `strings` (liste). Tomme samlinger skal være med. Egne flagg deklareres slik: `"call_help_done": {"default": false, "purpose_nb": "Samtalen er ferdig."}`. Delte flagg deklareres i pack.json, ikke dupliseres.

En scene har:

```json
{
  "id": "D01_HANDOVER",
  "phase": "handover",
  "channel": "phone",
  "mode": "live",
  "entry": "trigger",
  "trigger_nb": "Telefonen ringer etter parkering ved SARO.",
  "requires": {"all": ["intro_arrived"], "none": ["handover_done"]},
  "direction_nb": "Kort overlevering, vanlig kollega.",
  "lines": [
    {"id": "D01_001", "speaker": "DALE", "text": "You made it.", "delivery_nb": "Varm og tørr."}
  ],
  "choices": [],
  "on_complete": {"set_flags": {"handover_done": true}, "give_documents": []}
}
```

Alle linjer har globalt unik `id`, `speaker`, `text` og `delivery_nb`. Valg har globalt unik `id`, engelsk `label`, `next` (scene-ID), `requires` og `on_select` med samme format som `on_complete`. Ingen tomme eller eksterne `next`-referanser. Interne svarscener bruker `entry: "choice"`; startscener bruker `trigger`. Scener kan ha valg og avsluttes da først etter at valgt svar er returnert; bruk overordnede fullføringsflagg på riktig avslutningsnode. Dialogens returgang til et fysisk oppdrag beskrives i `trigger_nb`/integrasjonsnotatene. Unngå endeløse obligatoriske samtaleløkker.

Faser: `commute`, `handover`, `early_shift`, `signal`, `archive`, `field`, `all_night`. Kanaler: `physical`, `phone`, `walkie`, `radio`, `ui`. `mode` er `live` eller `fixed`. Faste avtrykk har ingen svarvalg og kan ikke besvare nye spørsmål.

Dale bruker Kinski-profilen **bare** via `phone`, `walkie` eller `radio`. Han har ingen fysisk opptreden og ingen Kinski-forteller på VHS. Ward er egen vakthavende. Spilleren er taus; spørsmål uttrykkes som valgtekst, ikke innspilt spillerstemme.

## Dokumenter, medier og tekster

Dokument: `id`, `title`, `kind` (`hand`, `typed`, `photo`, `map`), `stamp`, `body` (liste med engelske avsnitt), `production_nb`. Alle synlige merknader finnes i body; production_nb vises ikke i spillet.

Media: `id`, `title`, `duration_seconds`, `production_nb`, `beats`. Hvert beat har `at`, `until`, `picture_nb`, `audio_nb`, `subtitle` (engelsk, gjerne tom når ingen tekst trengs). Hele varigheten skal være dekket i rekkefølge uten hull/overlapp. Filene skal produseres som faktisk bevegelig film med lyd, ikke stillbilder. Ingen lydfil/video er laget av JSON-en.

Tekst: `id`, `text` (engelsk) og `context_nb`. Gjenbruk tekster gjennom ID-er. Ikke legg interne implementeringsord inn i produkttekst.

## Felles identiteter og flagg

Aktører i pack.json: `DALE`, `WAITRESS`, `RADIO_HOST`, `WARD`, `ATTENDANT`. Ikke bruk Dale/Kinski som generisk radiovert. Felles flagg: `intro_arrived`, `intro_completed`, `food_bought`, `food_eaten`, `food_taken`, `handover_done`, `routine_checked`, `trafo_normal_checked`, `trafo_anomaly_checked`, `office_key_found`, `office_open`, `office_evidence_taken`, `vhs_seen`, `vhs_contact_saved`, `record_compared`.

Eksterne leseflagg: `ctx_signal_present`, `ctx_archive_open`, `ctx_nora_contacted`, `ctx_all_night_active`, `ctx_power_available`, `ctx_room_quiet`, `ctx_map_solved`. Det er oppdraget/adapteren som setter disse, ikke manusets on_complete.

Kontordokumentene eies av office.json: `EV_HANDOVER`, `EV_SERVICE_LOG`, `EV_RADIO_SCHEDULE`, `EV_DALE_INDEX`, `EV_DALE_CLIPPINGS`, `EV_DALE_CONTACT`, `EV_DALE_NOTE`. Medier: `VHS_DALE_FIELD_01` og eventuelt ett kort tillegg. Disse ID-ene kan refereres fra andre moduler.

## Kanon og tone

Sen ettermiddag 13. april 1986, skiftstart 18:00 ved SARO. Den foreslåtte nye residualen er 00:13:47 den 14. april. Finalen er 05:29:00 til 05:29:47; 1947-opptaket beholder sin egen tid. 1420.405 MHz, 27 antenner, az 026. 1420 AM er vanlig lokalradio, ikke samme RF-frekvens. STATION 01/Nora/C og byens A holdes adskilt. Kontoret avslører ikke hele 1947-fotografiet, C-krysset eller slutten. Ingen hopp ut av skap, kryptisk dialog i hyggelig åpning eller TV-monolog som forklarer alt.

VHS viser et tvetydig, bevegelig avvik ved en gjenkjennelig kraftlinje/trafo. Dales håndskrift kan forklare en vanlig kontroll og et ståsted, men han kjenner ikke forfatterfasiten. Dale er bare på telefon/radio/walkie også når hans stemme opptrer som et fast avtrykk.

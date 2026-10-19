# Kinski: alle fortellerbeskrivelser

Direkte bestilt av Tom 19. oktober 2026. Gren `codex/kinski-descriptions`, base main `95719a4`. Prosjektbibliotekets VoiceStudio-oppsett og eksisterende lokale profil er gjenbrukt. Ingen nye avhengigheter er installert.

## Lyd og tekst

PASS: 1555 MP3, 18 791 ord, 45 411 332 byte og 5541,30 sekunder. 1218 klipp er nye; de 337 tidligere MP3-filene og katalogradene er faktisk SHA-256-sammenlignet med Gitbase og er uendret. `asset-audit.json`, `../verification.json`, `../generation.json` og `../provenance.json` har kilde, forespørsler, hasher og målinger. Ingen manglende eller foreldreløse klipp, alle `dropped_chunks=0`, ingen dekodede samples klipper. Kildelyd, profiler, WAV-cache og modeller følger ikke runtime.

Motoren prioriterer fulltekst, ellers en komplett kjede av innspilte tekstbiter. Tallord bevarer klokkeslett, fortegn, desimaler og innledende nuller. Ukjente ord avviser hele teksten og viser manglende opptak. Kartets 22 korte formuleringer beskriver eksisterende kartfakta. Hovedspillets dialog og dokumenttekst beholdes ordrett.

## Sluttkontroll

| Kontroll | Resultat |
| --- | --- |
| TypeScript, Pages-bygg og enkeltfilbygg | PASS |
| Avspiller og dynamisk resolver | 55/55 PASS |
| Tekstkorpus, uten nettleserlyd | 26/26 PASS |
| Beskrivelser i Pages-undermappe | 115/115 PASS |
| Beskrivelser i enkeltfil med streng CSP | 116/116 PASS |
| Tidligere opplesningsprøver, Pages / enkeltfil | 37/37 og 38/38 PASS |
| Telefonprøver, Pages / enkeltfil | 49/49 og 50/50 PASS |
| Telefonmotor | 28/28 PASS |
| Walkthrough | 57/57 PASS |
| Kapittel 1, 2, 3, 4, 5, 6 | 75/75, 76/76, 72/72, 69/69, 55/55, 42/42 PASS |
| Utviklermeny, kjøreturer, naturlyd, diner | 14/14, 23/23, 7/7, 15/15 PASS |
| Nærblikk, bevisbord, album | 42/42, 109/109, 59/59 PASS |
| Musikk, Pages / enkeltfil | 12/12 og 5/5 PASS |
| Lagring, styring og CSP | 19/19, 5/5 og PASS |

`*-results.json` angir faktiske kommandoer, utgangskoder og tider. Alle tunge nettlesertester kjøres sekvensielt gjennom prosjektets kø med GPU-oppsettet, etter at TTS/ASR er avsluttet. `regression-payload.json` og `../build-manifest.json` identifiserer kilde og begge testede bygg.

`tools/descriptions.py` kontrollerer 221 beskrivelser ved start og 270 senere i historien, i tillegg til faktisk synlig UI. Den bruker ekte dekodede MP3-filer og prøver i hovedlydbussen. Ekte tastetrykk, museklikk, raycast med Zoom, kartdrag, albumbytte og mobiltrykk er med. Spilltilstander for senere kapitler settes opp gjennom spillets testgrensesnitt; dette er ikke en gjennomspilling av alle mulige valg. Streng CSP tillater ingen eksterne MP3-forespørsler i enkeltfilbygget.

Prøvene dekker korrekt tallkø for veggklokken, fem mottakerkort, dynamiske hint og svar, skjult tekst, manglende opptak, telefonprioritet, stopp ved filmsekvenser, 650 ms nærblikk, frigitt Zoom, V, lukking, fane-/foto-/tekstbytte og albumknappens Enter/Space. Mobil ved 844 x 390 har reelt trykk som starter MP3, ingen vannrett overflyt og ingen kollisjon mellom leseknapp og klokke/Notes/berøringskontroller. `d01` til `d07` viser kart, mobil-Kit, nærblikk, mottaker, bord, foto og mobil-HUD. Disse er visuelt kontrollert.

## Historikk og grenser

Tidlige feillogger i `attempt-*` og `compatibility-attempt-1` er bevart. De fanget to manglende kildetekster, HUD-pekere, klokkeløs notatvariant, albumets testoppsett og en gammel testselektor. En senere revisjon fant at manuell HUD-lesing kunne fortsette inn i en filmsekvens. Dette ble rettet før den siste komplette kjøringen. Tidligere beståtte kandidater ligger i samtalens lokale bevismappe. Ingen av disse gamle loggene er sluttresultatet.

Store kapittelbilder ligger lokalt i `/home/tom/Documents/Codex/2026-10-19-lurer-p-lage-en-voice-clone/outputs/signal47/descriptions-integration-qa/`; logger, JSON og sluttbildene for den nye funksjonen følger repoet. Tidligere telefonleveranse har egne bevis i `../qa/`.

ASR-prøvene er automatiske ordkontroller med dokumenterte avvik rundt bokstaver, homofoner og korte tallord. De bekrefter ikke opplevd klang eller stemmelikhet. Menneskelig lytting, sinne/likhet, fysisk lyd fra PC/mobil samt fps og minne er UNVERIFIED. Analyserprøver viser Web Audio-signal, ikke høyttalertest. Ingen genereringstjeneste trengs under spilling.

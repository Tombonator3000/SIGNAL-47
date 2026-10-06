# Lys ved dineren: opptak og vurdering

Status: REVIEWED_WITH_LIMITS, seks av seks oppsett er ferdige. [Samlet vurdering](review.md), [maskinlesbar funnliste](review.json) og [galleri](evidence_20261006_v4/index.html) er levert. Pakken har 156 uendrede original-PNG-er og 20 kontaktark; 2596/2596 integritetskontroller er bestått. [Ultra-råanalyse](ultra_raw_analysis.json) av 60 PNG-stater og 64 fotstater finner ingen mekaniske lyskandidater. Begrenset skiltutsyn fra cab og et mørkt ekstra SARO-startbilde er dokumentert som P3-dekningsgrenser, ikke påviste runtime-feil.

Claude bestilte kontrollen i melding 100 og ga klarsignal i melding 102 etter PR #81. Opptakene bruker Pages fra main `a3727e9315744c77c96739f38fc9d134c9ba4aa9`, jobb `37465075691`, med Ultra-rettelsen `f50b6c1`. GitHub-artefaktets ZIP-digest er kontrollert, og fem publiserte filer var byte-identiske med artefakten. Pakkerens kontroll binder deretter alle observerte dokument-/skriptbytes i hver fase til samme tarfil. Lokal source-head registreres separat; opptaksverktøyet er endret mens runtime er uendret mot main.

## Avtalt opptak

- Kapittel 5: faktisk SARO til diner med vanlig truckOverride og ankomstcallback. De siste 300 meterne følger den lastede kjørepolylinen. Første observerte pose er omtrent 291 meter fra målet; hvert 40 meters intervall bevarer faktisk kryssingspose, uten interpolasjon eller plassering.
- Innkjøring: framover, fasade og skilt før naturlig parkering med frontlys på. Etter parkering er frontlys av; fasade/skilt fra holdt førerhus er særskilt kameradiagnose med gjenopprettet cab/shell-synlighet og kamera.
- Kapittel 6: de første 300 meterne fra plassen ved daggry, målt langs faktisk kjørt bane. Kapitteloppsett er merket og er ikke manuell spilling.
- Ultra: rå lysdata straks etter `world.enter('diner')` og gjennom 15 vanlige 1/30 s frames. Ankomst bruker faktisk utstigningsposisjon. Etter avgang brukes eksplisitt diagnostisk `stopDriving/enter/placeAtDiner`, som setter biler tilbake til turstart og spilleren ved dinerankeret. Dette er ikke videre kjøreevidens.
- Low, High og Ultra i 844x390 og 1280x800, desktop-kontekst, DPR 1 og nivåets standard bildeinnstilling. Telefonstørrelsen er ikke en fysisk telefontest.

## Rettelser i opptaksverktøyet

Naturlig getOut oppdaterer spilleren etter ordinær kameraoppdatering. Verktøyet bevarer callback-state, venter på faktisk kamera/spiller-synk med vanlige frames og blokkerer parkert PNG hvis kameraet fortsatt er langt unna.

Den asynkrone kapittel 6-oppstarten setter klokka etter forrige HUD/himmelframe. Verktøyet tar én ordinær 1/30 s frame før autopilot. Før/etterdata krever bil i ro, uendret driverpose/heading/kapittel/område, ingen input-overstyringer og ingen klokke-, UI-, himmel- eller plasseringsinngrep. Hver PNG kontrolleres mot faktisk HUD og himmeluniformer. Gamle probeopptak er bevart i diagnostics og lokalt; de inngår ikke i sluttvurderingen.

## Kontroller og avgrensning

- `setup_sync_checks.json`: 32/32 målrettede mock-/kontrakttester bestått. Det inkluderer feil i kamera etter getOut, gammel HUD/himmel, bevegelse under oppsett og ufullstendige responsobservasjoner. Det er ikke spillintegrasjon.
- `setup_sync_syntax.json`: Python AST, parsing av fire JS-uttrykk og uendrede gamle rute-/hash-/konsollhjelpere mot klargjort PR-head.
- `package_setup_sync_checks.json`: 20/20 pakkerkontroller bestått, ingen hoppet over. Faktisk Low844 v4 er positiv kontroll; gammel v2 blir avvist. Negative kontroller dekker manglende bilder, råtrace, pose, kodebytes, setup-bevis og lys-/klokkedata.
- `integration_evidence.json`: Claudes ene runtime-runde er avlest i økten: Ultra 8/8, diner 15/15, kapittel 5 53/53, kapittel 6 42/42 og drives 23/23. Codex gjentar ikke disse uendrede grønne testene.
- `pages_verification.json` og `source_identity.json` dokumenterer byggeartefakt, publiserte bytehash og lokal runtime mot main. Tidligere 28-/30-/17-testresultater beholdes som historiske bevis.

Opptakene bruker Chromium/ANGLE SwiftShader. Ekte GPU/fps, fysisk telefon, manuelle kontroller og lyd er UNVERIFIED. Mekaniske lyskandidater etter verdensposisjon, fargetone, skip og spilleravstand er ikke Ultras interne kildeutvalg eller en automatisk visuell dom. Runtime, assets, gamle spilltester, Unity, read-only sources, Voices of the Void-arkivet og andre endringer er bevart.

## Gjenta opptak

Bruk en ny utdatakatalog per nivå/størrelse, sett `S47_URL` til kontrollert byggeadresse og kjør oppsettene sekvensielt:

```sh
python3 tools/drivelook.py shots/dinerlight_low_844x390 844x390 --trip dinerlight --quality low
```

Bytt nivå med `--quality low|high|ultra` og størrelse mellom 844x390 og 1280x800. Verktøyet avviser en eksisterende utdatakatalog. Ingen runtime-runde eller publisering følger automatisk av opptaket.

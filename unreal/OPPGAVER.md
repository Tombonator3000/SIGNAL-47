# Oppgaver og arbeidsdeling (Unreal)

Felles liste for Claude (integrator og hjelpere) og Codex (ChatGPT). Hver oppgave har eier, gren og status. Les denne før du starter, og oppdater status når du begynner og når du er ferdig.

## Arbeidsdeling

- Codex: rendering, lys, skygger, materialer, post-prosess, importløypa for materialer.
- Claude-hjelpere: spillogikk, grensesnitt, lyd, plassering av modeller i verden.
- Claude (integrator): fletter inn i `unreal-port-core`, kjører kort testutvalg, pakker og oppdaterer `~/Games/Signal47`, holder logg og denne lista.
- Gjennomgang: når en side leverer, ser den andre over koden og bildene før fletting. Codex' leveranser gjennomgås av Claude og rettes i samme Codex-økt (`codex exec resume`); Claude-hjelpernes leveranser kan sendes til `codex review`.
- Filansvar: rør bare filene oppgaven gjelder. Delte filer (`S47HUD.cpp`, `S47Main.*`, `S47Story.*`, `S47Web.*`, `S47World.*`) endres minimalt. Ta inn `unreal-port-core` i grenen din før du leverer.
- Tester: det som er endret og `-S47SelfTest`. Ett Unreal-program om gangen per tung jobb: sjekk `pgrep -fa "UnrealEditor|UnrealBuildTool|Signal47"` før start.

## Pågår

| Oppgave | Eier | Gren | Status |
|---|---|---|---|
| Grafikkpass: PSO-cache og innlastingsskjerm, lys/skygger/materialer i alle områder, før/etter og pakket 1440p-måling | Codex | `port-gfx`, WIP `64d943e7` + merge `48b1ea48` av `93ab1568` | pågår 23.10; startup-feil rettet, CRT-glass og 118 materialprofiler, sekvensielt måle-/PSO-verktøy og minnevern klargjort. Lette kontroller består. Tom spiller installert PID 7609; bygg/import, bundlet cache, selvtest, alle før/etter-bilder og 1440p/oppstartsmåling er UNVERIFIED til ledig testperiode. Ingen ny pakke/installasjon. Se LIGHTING.md øverst. Claudes props urørt. |
| Grafikkpass: plassering av gratismodellene (S47Props, Config/Props/<område>.json), logoer bort; først Toms politibil | Claude-hjelper | `worktree-agent-a6660375fa9508a18` (fra WIP 36a06dc6) | klar 23.10 for gjennomgang: sheriffbilen (LT '80 Cop Cruiser, brun/hvit med stjerne, førerdøra åpner seg, rød/blå blinkende lysbjelke med to små spotlys uten skygger), DP-80 med animert utskrift, radiobord (CB-base, 3278, spolebåndopptaker), kassettopptaker, skrivemaskin og telefon i arkivet, motellseng i rom 6, kraftlinje med ledninger og avgrening til SARO, piggtrådgjerde, steiner, busker, yucca, tumbleweed, klipper, vrak og knekt stolpe, MOTEL-skilt, biler og telefonkiosk ved motellet, biler i Caliche og ved dineren; logoer malt bort i repaint.py; selvtest 1220 OK; C10 som SARO 07 ikke gjort (se rapporten) |
| Brettet: synlige røde tråder (kantutjevnet garn med skygge, glans og tvinning, større nåler, slakk feiltråd, tråden i hånden med ring på målet), hjelpestripe, tydeligere spørsmålskort, hint tilbudt etter feil tråd eller 75 s, «Slik lages et funn» i notatbokas funn, oppgavetekster nevner brettet; testflagg `-S47BoardTest` | Claude-hjelper | `worktree-agent-a6671d6d8e7c95ad4` | levert 22.10, venter på integrator |
| SAROs utelys og fotolab: fysiske lamper ved vegger/dører/stier, navngitte normal-/safelight-/forstørrerlys | Codex | `port-light` | klar 22.10 for Claude-gjennomgang; flettet til 3b44956e; 30 fysiske utelys, navngitt lab-API koblet til dørregelen; bygg og 1218 selvtester OK; bilder i LIGHTING.md; ny 1440p-måling og pakking hos integrator |
| Toms rapport: mørkerommet krever lukket dør (tåket kopi ved åpen dør, rødt mørkeromslys, lagring), og signalprosessoren gjort varm (bånd synker i tone, lavpass, ingen tynne overtoner, RAW lavere) | Claude-hjelper | `worktree-agent-a0f1f30ea2f5f2b7a` | ferdig 22.10, klar for fletting: bygg OK, selvtest OK (1186), lydtur før/etter i loggen; mørkerommet ikke prøvd i verdenen |

## Neste

| Oppgave | Eier | Merknad |
|---|---|---|
| Grafikkpass med de 70 gratismodellene (`~/Assets/signal47-free`, `MODELLER.md`): pickup, biler, kontor- og radioutstyr, diner og motell, ørken; logoer fjernet; CREDITS i rulleteksten | Claude-hjelpere (plassering), Codex (materialer og lys) | Import ferdig 22.10 (gren `worktree-agent-a82b7e58ccdd2c41b`): `Tools/freemodels/import_free.sh` gir 246 statiske mesher (70 modeller, pakker delt per gjenstand) i `/Game/Free/<kategori>/<modell>` (ignorert, 2,8 GB, teksturer kokes til 2k), ekte størrelse og pivot midt under, Nanite på 12 tunge opake; oversikt i `Content/Free/free_models.json`; testflagg `-S47FreeModels[=navn,...]`. Kreditering i rulleteksten (68 s) og `CREDITS.txt` ved siden av spillet. Gjenstår: plassering etter lyset, `/Game/Free` i `DirectoriesToAlwaysCook` da, logoer (Chevrolet på C10-grillen, IBM, Sony, Cobra) |
| Matriseskriver og motellseng (finnes ikke gratis) | Claude-hjelper | ferdig 22.10: laget i Blender, `~/Assets/signal47-free/own` (glb, fbx, blend, README); plassert 23.10 (S47Props: skriveren i kontrollrommet, senga i rom 6) |
| Lyttetest av lyd og hel gjennomspilling med ekte input | Tom | |

## Ferdig 22.10

Menyer, grensesnitt i spillet, pipetonen, møll, 02:15-kameraet, terminalene på skjermene, musepeker på brettet, ut av bilen overalt, hopp, fri ferdsel, Unarius-filmene, den kjøpte signalprosessoren, tonegåten (TAPE A / 1947).


KELVORA ferdig av Codex etter Toms «Plasser i spillet», 22.10 kl. 23:06: egne TV-er på motellkontoret og i rom 6, RANGER 640 på SAROs arkivskap. `/Game/Kelvora` har 18 egne assets, tre LOD-er og kollisjon. Filmene er levende med lyd; eget unlit skjermmateriale med korrigert FBX-V, bevart film-/strømkontrakt og pilot som følger strømmen. Gamle visuelle deler er skjult, kontorets sammenslåtte TV-geometri er klippet uten å ta hylla. 38 Blender-eksportkontroller, import, editor/Linux-bygg, 1218 selvtestsjekker og ni scenarioer består. Etter siste materialrettelse er selvtest, begge filmer og rom 6 av/på prøvd på nytt. Full 47-sekunders opptaks-/lagringsprøve består på samme geometri og kode før den siste V-korrigeringen. Installert i `~/Games/Signal47`, binærfil og tre pakkefiler hashkontrollert mot kandidaten, 27 save/config-filer uendret. Bevis i `~/scratch-bm/Signal47-kelvora`, arbeidsbeskrivelse og oppsummering i `Signal47/Tools/kelvora/`. Ikke push eller PR.

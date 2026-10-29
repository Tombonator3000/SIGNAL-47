# Gjeldende arbeidsliste

**Status:** Gjeldende. **Gjelder:** Videre arbeid på Unreal-utgaven. **Sist kontrollert:** 29. oktober 2026.

Dette er den samlede aktive listen. Faktisk leveringsstatus står i [STATUS](STATUS.md), gjeldende føringer i [DECISIONS](DECISIONS.md), og filansvar i [OPPGAVER](../../unreal/OPPGAVER.md). En oppgave lukkes med konkret resultat og nødvendig bevis; en rapport eller en ferdig deltest lukker ikke resten av leveransen.

## Stabilitet og feil fra spillprøven

Produksjonsverktøy: første versjon av [koordinatoren med Claude](Guides/PRODUCTION_SYSTEM.md)
er implementert. Den ordinære live-workerprøven og en faktisk tung verktøyserie
står som avgrensede kontroller når henholdsvis Claude-kvote og et konkret
produksjonsbehov gjør dem aktuelle. Dette lukker ingen spilloppgaver nedenfor.

| ID | Åpen oppgave | Underlag |
| --- | --- | --- |
| FIX-01 | Delvis levert: [x] Linux-minnetiltak PR215, pakket R1 full diner/retur og natt-R2 uten override PASS. [ ] Avklar første historiske High/frame0-OOM og eventuell samtidig prosess. | [Målt minnetiltak og bevisgrenser](../Evidence/UnrealDeliveries/DINER_VULKAN_2026-10-29.md). Ingen fullmatrise/installasjon. Ny TB2-kilde er ikke i denne R1-pakken. |
| FIX-02 | [ ] Rett Motor Courts gangnivå, svevende potter/stoler og usynlige kollisjonsvegger foran dørene. | Spillerbilder E02, E04 og E05 i [bevisgalleriet](../../unreal/Evidence/Playtest-2026-10-28/README.md). |
| FIX-03 | [ ] Rett doble mottakslamper og kontroller faktisk bevegelig film/lyd på resepsjons-TV-en. | Bilde E03 og Toms observasjon. Stillbildet alene beviser ikke filmstans. |
| FIX-04 | [ ] Rett bassengsti som blafrer, bassengkant/dekke under terreng og uønsket klump utenfor gjerdet. | E04/E05. Identifiser det konkrete uønskede meshet før fjerning. |
| FIX-05 | [ ] Rett telefonretning og overlappende ICE-/avis-/skilt-/veggdeler ved motellet. | E05. Bevar ønsket retro keramikkmønster. |
| FIX-06 | [x] (PR214, kilde, SC01-kontrakt PASS 190) Behold museposisjonen ved ny SARO-dialog, flytt tekstpanelet ned og rett veiledning/fremdrift ved service-/besøksdør. | [Detaljert feilrapport](Reports/PLAYTEST_TODO_2026-10-28.md). Ny dørtekst på main er ikke alene bevis på installert eller fungerende løsning. |
| FIX-07 | [ ] Erstatt gjenværende enkle/svevende trafodeler og integrer rampe, grus og fundamenter naturlig i bakken. | E06. Bevar service, strøm, lyd, adgang og SC02. |

## Omgivelser og mennesker

| ID | Åpen oppgave | Underlag |
| --- | --- | --- |
| ENV-01 | [x] (PR216, native: import 1280/1280, assets 269/269, LOD 588/588, town-visninger PASS med 15 gjennomsette bilder; pakket spill UNVERIFIED til neste fullmatrise) Integrer TB2s sheriff, vaskeri, feed, kino og kirke helt frem til native og pakket spill. | [Bygningskilder](../../unreal/Signal47/Tools/townbuildings/README.md). Blender-kilde alene er ikke levering i spillet. |
| ENV-02 | [ ] Oppgrader vanlige Caliche-hus og kontroller synlighet, retning og plassering av byskiltene ved innkjøringene. | [Feilrapport](Reports/PLAYTEST_TODO_2026-10-28.md). Bolighusene er ikke med i TB2-pakken. |
| ENV-03 | [ ] Gi trafikksignalet mer detaljert hus/oppheng og svak vindbevegelse, med bevart nattlys. | Toms feilrapport, punkt 3. |
| ENV-04 | [ ] Bekreft farmen fra E01 mot koordinater og gjør vei/port/adkomst forståelig. | Kildeundersøkelsen peker mot statisk ranch med stengt port, uten implementert besøksinteraksjon. |
| HUM-01 | [ ] Lag sittende radio-kvinne med passende klær og bevart radio, proxy og tilstedeværelse. | [Mannequin-plan](../../unreal/Signal47/Tools/metahuman/mannequins-plan.json). Kvinnelig prototype finnes lokalt. |
| HUM-02 | [ ] Lag mannlig MetaHuman-grunnvariant og bak stående røykende mann og sittende diner-sjåfør. | Ferdig mannlig kilde og alle nye poser/baking mangler. Bruk statiske positurer med delte spillmaterialer og LOD. |
| HUM-03 | [ ] Poler senere servitrisens stoff, portrett/utseende og servicegester. | [Levert figurpilot](../Evidence/UnrealDeliveries/METAHUMAN_WAITRESS.md). Behold fungerende figur og dialog. |

## Grafikk, lys, lyd og effekter

- [ ] **GFX-01:** Prøv Nanite selektivt på faktiske oppgraderte, tette solide miljømodeller. Bevar RT-fallback, lysblokking og gameplay-kollisjon. Behold godkjent terreng/himmel.
- [ ] **GFX-02:** Forbedre armaturmontering, kildeplassering, rekkevidde og lokal skyggekostnad ved Caliche, motellet og trafoen.
- [ ] **GFX-03:** Forbedre stedlig støv, gnister og vær med riktig feste, bounds, levetid og kvalitetsstyring. Bevar eksisterende trafo-lysbue.
- [ ] **GFX-04:** Forbedre kildeplassering, avstandsdemping, vegg-/dørdemping og romklang, tilpasset dagens egen lydgraf.
- [ ] **GFX-05:** Prøv sammenhengende 1986 New Mexico-farger gjennom dag, skumring og natt. Avklar egen tonekjede før eventuell AgX-/ShaderShift-bruk.
- [ ] **GFX-06:** Vurder senere lokale vegetasjonsskygger og Lumen Lite når en konkret måling eller kvalitetsmangel begrunner tiltaket.

Ferdigheter og avgrensning: [Unreal-kildegjennomgangen](../Research/Unreal/UNREAL_SKILLS_2026-10-28.md) og [tidligere renderundersøkelse](../Research/Unreal/RENDERING_RESEARCH_2026-10-26.md). Dette er forslag til produksjon og vurdering, ikke dokumenterte ytelsesgevinster.

## Historie og samlet spillerreise

- [ ] **STORY-01:** Fullfør den nye kveldsrekkefølgen: forenklet SC01, signal/strømsvikt kl. 00:13, mørk trafotur, senere SC02 og sammenhengende retur. Claude/Codex-fordeling står i [kveldsplanen](../Design/Story/KVELD_PLAN_2026-10-28.md).
- [x] **STORY-01A, Codex-del:** Ny nattrekkefølge, vedvarende trafofeil og SKY01 gjennom PR209. Pakket R2 består 74 nattkontroller med åtte diskrestores og 14 Arc/calm-kontroller; naturlig SKY01, grafisk sikt og fersk Continue er prøvd separat. [Rapport og gjenværende bevisgrenser](../Evidence/UnrealDeliveries/NATTSEKVENS_2026-10-28.md). Samlet STORY-01 og faktisk SC03 er fortsatt åpne.
- [ ] **STORY-02:** Fullfør faktisk SC03 med telefonmelding, Ortiz, servicevogn 23, termose, bortføring og etterspor. Deretter SC04/SC05 som samlet aktleveranse etter [revidert regi](../Design/Story/HISTORIE_SC02_SC13_REVISJON.md).
- [ ] **STORY-03:** Produser SC06 til SC13 med nødvendige verdenshandlinger, modeller, stemmer, lagring og progresjon. Bruk [implementeringsplanen](../Design/Story/HISTORIE_IMPLEMENTERINGSPLAN.md) under de senere revisjonene, ikke gamle antakelser om ferdig spilletid.
- [x] **STORY-04A, kilde:** Fire ekte arrayobservasjoner 19:00 til 22:50 med papirer og hint er flettet i PR206. Pakket/installert levering føres separat i STATUS.
- [x] **STORY-04B, kilde:** SC01 med klask mot ruta i stedet for hånd, avtrykk når skjermen heves, og lampetest bare innenfra. Fem ekte observatorielyder med kreditering i arrayarbeidet (PR208). SC01-kontrakt PASS, 157 kontroller.
- [x] **STORY-04C, kilde:** Aggregattest 19:35 på fjernpanel og runde til arraygjerdet 21:40 der porten står åpen igjen (PR210). SC01-kontrakt PASS, 176 kontroller.
- [ ] **STORY-04:** Viderefør kveldsarbeidet: nye dispatchopptak for SC01_FIRST_REPORT_001/002 (ny tekst). Ikke legg inn tidlige alienmøter som bryter den nye regien.
- [x] **STORY-04D, kilde og målrettet kontroll:** Ett SKY01-lys starter naturlig ved 21:30 og fortsetter over kveldens korte hopp. Fem himmeltrinn består 24 grafiske sikt-/lagringskontroller med faktisk feltkamerabilde. Inngår i PR209, uten installasjon.
- [x] **STORY-04E, kilde og målrettet kontroll:** Dales gjenbrukte stemme ved den tomme besøksinngangen, lagret hendelse og stopp ved sak/talebryter/områdebytte. Editor: 1709 selvtester, SC01 185/19 og målt romlig PCM. [Kvittering](../Evidence/UnrealDeliveries/DOOR_VOICE_2026-10-29.md). Ingen ny pakke/installasjon.
- [ ] **FX-01:** Produser den korte meteorpassasjen ved stjernehullet med riktig hendelsestid og engangsavspilling. [Design og kilder](../Design/Proposals/NIAGARA_VFX_2026-10-28.md).
- [ ] **FX-02:** Produser den store UFO-avsløringen med cloud tank-skyer som del av finalen, med bevart THE EVENT og 1947-bilde. [Finaleretning](../Design/Proposals/FINALE_CLOUD_TANK_UFO.md).
- [ ] **ART-01:** Integrer ferdige plakater, bilder og dokumentrekvisitter på riktige flater med lesbare UV-/materialbindinger. Bevar fungerende dokument-ID-er og bevisgrunnlag.

## Levering og kompatibilitet

- [ ] **SHIP-01:** Verifiser Win64-kompilering, cook, oppstart, lagring/lasting og normal/Low-spilling før en Windows-utgave ferdigmeldes.
- [ ] **SHIP-02:** Undersøk faktisk kompatibilitet og ytelse på mottakermaskiner. Linux-/Steam Deck-/gamepad-/lytteaksept hevdes bare med egne faktiske prøver.
- [ ] **SHIP-03:** Ved neste autoriserte samlede levering: før testet kandidat, main-merge, installasjon og eventuell offentlig nedlasting separat i [STATUS](STATUS.md). Bevar personlige Saved og testbevis.

## Eldre, uavklart restliste

Den tidligere lange [unreal-todoen er bevart som historisk restliste](../Archive/Superseded/Unreal/todo.md). Den blander gamle portkroker, utførte leveranser og åpne ideer. Uboksete rader der er derfor **ikke automatisk bekreftede feil i dagens spill**. De er heller ikke slettet eller ferdigmeldt: ved konkret videreføring kontrolleres faktisk kode og siste kvittering, og relevante funn løftes hit med et tydelig oppgave-ID.

## Arbeidsregel

Test etter risiko og samle sammenhengende leveranser. Dokumentendringer trenger lenke-/vedlegg-/diffkontroll, ikke motorbygg. En liten PR utløser ikke fullmatrise; en nødvendig samlet installasjons-/releasekontroll kjøres én gang. Bruk avgrensede subagenter med tydelig filansvar. Én tung jobb om gangen, og ingen test samtidig med Toms spilløkt.

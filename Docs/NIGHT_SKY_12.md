# NightSky12 — ny himmel, testkandidat

11. september 2026. Avgrenset videreføring fra `main` 1db7b41, etter merge av Visual10 / PR7. Kildecommit: `94811347a00811648f937400a1522c7d2cdd1d2b`.

**Leveransen er lokal.** Siste direkte GitHub-kontroll svarer `private: false`, `visibility: public` for Tombonator3000/SIGNAL-47. Dette avviker fra prosjektinstruksens beskrivelse av et privat repo. Ingen push eller PR er gjort for NightSky12, siden det ville publisere de nye filene offentlig uten uttrykkelig autorisasjon. Repoets synlighet er ikke endret. Lokal gren, pakke og bevissett er klare til gjennomgang.

Spillets opprinnelige overlevering på 79c0742 er historikk. Første kapittel, fysisk framkalling, aktiv/passiv B-12-prøve og lagring er allerede levert i Visual10. Konsept11 «Den slettede natten» ligger fortsatt på `concept/story-sky-11`; den nye historien og himmelhendelsene er forslag. Denne kandidaten endrer den vanlige himmelen.

## Endring og kriterier

Det gamle stjernenettet tegnet 650 lyse firkanter i en kule 160 meter fra origo. Den nye himmelen bruker en 8192×4096 katalogtekstur med små stjerner, lysstyrke- og fargevariasjon og en dempet Melkevei. Himmelen følger synsretningen uten avstandsavhengig parallakse. Unity projiserer kartet til seks kubeflater ved import. Filtrering i synsretningen, sømløse mipnivåer og demping mot horisonten begrenser grove punkter og polfeil.

Fire kriterier ble satt før den visuelle kontrollen: finere stjerner og synlig Melkevei; tydelig antennesilhuett og lokale arbeidslys; fortsatt lesbare fotografiske spor og fungerende kapittel; etablert releaseytelse på referanse-PC-en. De siste to krever en ny spillerreise.

Original stjernemesh/materiale er bevart med tegningen deaktivert. Unity-scenens endringer er himmelmaterialet og denne rendererens aktivering. Den bakte labens to lysatlas er uendret. Spillkonstanter, interaksjoner, lagringskode, modeller og tidligere lisenser er beholdt. Ingen Magnific-generering er brukt.

NASA-originalen er kontrollert med SHA-256. Blender 4.5.13 gjør en dokumentert EXR→8-bit sRGB PNG-konvertering uten skalering; avledningen er 46 213 490 byte. Målt gjennomsnittlig absolutt feil i lineært lys er 0,00009915, maksimum 0,0043945. Originalen ligger i `Artifacts/Sky12/Source/`, og kan lastes ned på nytt av det reproduserbare skriptet. Den komprimerte Unity-kubeteksturen bruker seks 2048×2048-flater, BC7, 12 mipnivåer og trilineær filtrering. Dette er en kunstnerisk orientert himmel, ikke dokumentasjon av himmelen på et bestemt tidspunkt i 1986. Se [krediteringen](THIRD_PARTY_NOTICES.md).

## Produksjonskjeden

| Ansvar | Verktøy | Inndata → resultat | Nåværende tilgang |
| --- | --- | --- | --- |
| Kilde og revisjoner | Lokal Git / GitHub-lesing | main → egen lokal gren og dokumentert kandidat | VERIFIED WORKING; GitHub-repoet er nå offentlig, ingen push utført |
| Klargjøring av kart | Blender 4.5.13 CLI | kilde-EXR → 8K PNG og konverteringsrapport | VERIFIED WORKING |
| Spill og import | Unity 6000.3.22f1 / URP 17.3.0 | PNG → seks BC7-flater → scene → Linux-spiller | VERIFIED WORKING |
| Inspeksjon | Eksisterende fastkamera-verktøy i Unity | utpakket spiller → originale PNG-er og manifest | VERIFIED WORKING, statisk inspeksjon |
| Interaktiv kontroll | KDE / uinput / Input System 1.20.0 | virkelig tastatur/mus → kapittel og frame-måling | SETUP REQUIRED: lås opp skjermen |
| Valgfri ressursproduksjon | Magnific | bilde → GLB → Blender → Unity | Konto tilgjengelig; denne produksjonsveien AVAILABLE BUT UNTESTED |

## Hva som er kontrollert

| Kontroll | Status og avgrensning |
| --- | --- |
| Blender-konvertering, kildehash og Unity-import | PASS — faktisk utført; dimensjoner, format og konverteringsfeil registrert |
| Unity-scene, shader og Linux-release | PASS — identifisert Unity-kilde bygget; dokumentasjonsendringer registrert som working-tree, shader uten kompilasjonsfeil, to eksisterende lysatlas bevart |
| Statisk runtime-utseende | PASS i ni inspiserte sluttbilder — fire himmelretninger, senit, begge sider av +X/+Y-grensen, serviceområdet og vinduet; [egenvurdering og originaler](Evidence/NightSky12/final-9481134/README.md) |
| Ny tastatur-/musreise og fotografiske bevis | UNVERIFIED — KDE-skjermlåsen hindrer ekte input; faste inspeksjonskameraer teller ikke som spillerreise |
| Bevegelse, flimring og ytelse | UNVERIFIED — ingen ny representativ måling fra opplåst skrivebord |
| Pakkeinnhold og tidligere leveranse | PASS — 173 filer med hash, størrelse og rettigheter kontrollert etter utpakking; kandidatmerking på plass; tidligere arkiv og standardstarter uendret |
| Subjektiv lydmiks | UNVERIFIED, som tidligere; denne endringen berører ikke lydmiks |

Visuell kontroll er hovedagentens egenvurdering. Ingen uavhengige kritikere er kjørt i denne runden. Bildene er originale Unity-bilder med faste kameraer, uten retusjering. `--signal47-world-capture` og `--signal47-sky-capture` setter en eksplisitt inspeksjonstilstand; de dokumenterer ikke navigasjon, lagring, fotografering eller fps. Kontroll av fire himmelretninger, senit og begge sider av kubens +X/+Y-grense er statisk kontroll, ikke bevis for fravær av flimring i bevegelse.

[Bevisrapporten](Evidence/NightSky12/final-9481134/verification.json) knytter den eksakte pakken til kildehash, byggeutdrag, kamera-manifester, originalbilder og den faktisk kjørte negative testen av skjermlåsen. [Før](Evidence/NightSky12/final-9481134/Baseline/05-service-yard-array.png) og [etter](Evidence/NightSky12/final-9481134/Final/05-service-yard-array.png) viser samme kamerautsnitt. Terreng, nærmodeller og refleksjoner har fortsatt avstand til konseptmålet.

## Korrigering etter visuell kontroll

Den første equirektangulære shaderen ga en synlig radial utstrekking nær et katalogpolpunkt i bildet mot 270°. Den ble derfor forkastet selv om bygging og første oversiktsbilde bestod. Unitys kubeteksturimport erstatter den singulariteten med filtrering over seks flater. Første kodeforsøk brukte to importegenskaper på feil Unity-API-type; kompilatoren avviste det, og egenskapene ble flyttet til `TextureImporterSettings` før et nytt bygg. Kandidatbildet med feilen er bevart som avvik, ikke som godkjent sluttbilde.

## Skjermlåsen forklarte inputfeilene

Et nytt forsøk på den uendrede Visual10-pakken verifiserte alle 173 filer og startet spillet, men musekontrollen feilet. Direkte diagnose viste at XWayland kunne bekrefte spillfokus bak låseskjermen; neste virkelige musehendelse gikk til skjermlåsen. `org.freedesktop.ScreenSaver.GetActive` returnerte `true`, og KDE-låseprosessen var aktiv. En midlertidig hypotese om plassering på flere skjermer ble forkastet; ingen slik omgåelse er beholdt.

Native tester har nå en felles kontroll av skjermlåsen. Den kjørte negative testen stoppet før spillerstart/input, og bevarte eksisterende telemetri. Fokus kontrolleres også gjennom hver iterasjon av musebevegelsen. Brukeren er bedt om å låse opp; låsen er ikke deaktivert eller omgått. Feilede forsøk og diagnoser ligger i `Artifacts/Sky12/` og er ikke ommerket som bestått spilltest.

## Åpne og fullføre kontrollen

Gjeldende verifiserte utgave startes med `./Spill-SIGNAL47.sh` og er fortsatt Visual10. Ny kandidat:

`Artifacts/Releases/NightSky12-79cafef6f5a5/Start-SIGNAL47.sh`

Flyttbar pakke: `Artifacts/Releases/SIGNAL47-NightSky12-79cafef6f5a5-Linux.tar.gz`. `START_HER.txt` merker den som testkandidat. Den vanlige starteren og den tidligere pakken bevares.

Unity-kildehash: `79cafef6f5a59d042d101652cb5fb55942de4b13af91e8949a080d4caf580512`.
Byggpayload: `c04306ebfdfe5046170ff419acb4281dfb5fcdc90137d4a853dfadb453c8b51a`.
Arkiv SHA-256: `fc49644ab2a853d4afb2cf1e1bdc45425e3e3fa9d27039ff0260534cb379545d`.

```sh
bash Automation/build-night-sky12-linux.sh
# Etter opplåsing; bruk et nytt navn og en isolert profil for hver nye test:
python3 Automation/run-chapter09.py --name Sky12FreshActive --profile Artifacts/Chapter09/Profiles/Sky12FreshActive --player Artifacts/Releases/NightSky12-79cafef6f5a5/Signal47.x86_64 -- --phase all --method active --reopen --quit
```

Deretter kjøres passiv rute, faktisk prosessgjenstart og en separat målerunde med ferske bilder. Målekravene er fortsatt 1280×800 Ultra, Intel ARL/OpenGLCore, minst 20 sekunder oppvarming og 120 sekunder representativ bruk; snitt ≥59 fps, p95 ≤17,2 ms, p99 <20 ms og ingen intervaller over 50 ms. Ingen frameprøver skal fjernes. Visual10s historiske 73,412 fps / p95 15,368 ms gjelder den gamle pakken og overføres ikke til NightSky12.

## Bildene fra X og mulige bidrag fra andre verktøy

Det første vedlegget viser en scene med tette miljødetaljer, varme lyskilder og reflekterende overflater. Det er nyttig som komposisjons- og materialreferanse for SARO og et senere motell. Det andre viser en lesbar nattlig kjøresituasjon med vei, lyskastere og et opplyst stoppested; dette er særlig relevant inspirasjon til THE NIGHT ROAD, og senere veipartier i SIGNAL / 47. Dette er vurderinger av de vedlagte skjermbildene. Demoenes ytelse, kode, komplette spillbarhet og ressursrettigheter er ikke verifisert; Helion-lenken lot seg ikke åpne i nettleserverktøyet. Bruk prinsippene og egne/lisensierte ressurser.

[Dream Loop](https://github.com/achimala/dream-loop) beskriver målbildet, bygging, kritikk mot faktisk resultat og gjentatt korrigering. Gauntlet dekker allerede denne arbeidsmåten og legger til faktiske brukerreiser og målinger. Three.js kan passe små selvstendige nettspill; dagens SIGNAL / 47 fortsetter med Unity/Blender.

Magnific-koblingen og kontotilgangen er bekreftet. Den har bilde-til-GLB-verktøy; en konkret modell må deretter kontrolleres og bearbeides i Blender og importeres med riktige materialer, skala og kollisjon i Unity. Generering via denne koblingen bruker kreditter også når kontoens andre grensesnitt viser unlimited. Ingen kreditter er brukt i denne runden.

[Grok/X Search](https://docs.x.ai/developers/tools/x-search) kan bidra med å finne originalinnlegg, demoer og oppfølgingsinformasjon. [Claude Code](https://claude.com/blog/code-review) kan bidra med ekstra kodegjennomgang; en egen gjennomlesning av mysteriets årsakskjede og sammenligning av mål-/spillbilder er også nyttige oppgaver. Direkte Grok-/Claude-kobling er ikke bekreftet her, og ingen prosjektfiler er sendt til dem. Kontoabonnementene brukes ikke automatisk av denne oppgaven.

En avgrenset Grok-oppgave er å finne originalinnleggene fra @nelsonpatrao og @chetanankola med demo-/kildelenker, og skille forfatterens påstander fra det som kan prøves. En avgrenset Claude-oppgave er å kritisere om den faktiske spilleren kan slutte fra to fotografier til en begrunnet B-12-konklusjon, med konkrete moteksempler og uten å skrive om kanon.

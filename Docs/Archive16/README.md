# Archive16 — første fysiske dokumentprøve

Nyere modellpass: [Archive17](../Archive17/README.md) bygger videre på denne prøven. Bildene og pakkeidentiteten nedenfor er historisk Archive16.

Selvstendig Unity-prøve av P04 etter Recovery15. Spilleren åpner en fysisk mappe, leser original og korrigert protokoll, sammenligner dem og velger en slutning som kildene støtter. Dette er en produksjonsprøve med to faste kameravinkler, ikke et nytt tilgjengelig område i hovedspillet. Ingen lagring eller kapittelprogresjon inngår. Visual10 er fortsatt standardstarter.

## Ramme og aksept

Arbeidsgrunnlag: [P04/P05-papirprøven](../Menu14/PAPER_CASE.md), [designbibel v0.1](../DesignBible13/README.md) og de eksisterende SARO-materialene. Visuell referanse er det etablerte laboratoriet: dempet grønn metallinnredning, kremfarget papir, analog registrering og varmt arbeidslys. Den nye mappen er original Blender-geometri, ikke et bilde-til-3D-resultat eller nedlastet asset. Ingen nye genererte konseptbilder var nødvendige for denne avgrensede rekvisitt-/interaksjonsprøven.

Akseptkriterier for dette steget:

1. En mappe med riktig fysisk størrelse kan åpnes fra bordets inspeksjonskamera.
2. Begge faktiske protokolltekster og sammenligningen er lesbare ved 1280×800.
3. Sammenligning krever at begge dokumentvisninger er besøkt. Ubegrunnede slutninger gir tilbakemelding uten å registrere korrekt funn; spilleren kan prøve igjen.
4. Lukk/gjenåpne og eksplisitt nullstilling har tydelige tilstander, uten filskriving eller kobling til hovedspillets lagring.
5. En separat kandidatpakke åpner samme prøve fra en utpakket launcher, med kilde- og byggidentitet bevart.

Besøkt dokument er ikke bevis på at spilleren har lest eller forstått det. Prøven bruker foreløpig svaralternativer; det er ikke en blind forståelsestest av den endelige etterforskningsmekanikken.

## Verktøy og faktisk overlevering

| Ansvar | Verktøy | Input → output | Status |
| --- | --- | --- | --- |
| Rekvisitt | Blender 4.5.13 LTS, utført Python/CLI | Original oppskrift → redigerbar .blend → FBX | Bounds kontrollert ved ny Blender-import |
| Rom og interaksjon | Unity 6000.3.22f1 / eksisterende URP | FBX, SARO-materialer og protokolltekst → egen scene og Linux-spiller | Faktisk bygg og runtime-kontroller; se bevisrapport |
| Visuell inspeksjon | Originale Unity-captures | Faste kameraer og API-oppsatte dokumenttilstander → PNG | Selvkontroll, ikke native klikk |
| Levering | Eksisterende Python-pakker og eksplisitt launcher | Identifisert spiller → separat kandidatarkiv | Utpakking og faktisk launcher kontrolleres separat |

Mappen har 1080 trekanter og mål 0,46 × 0,3695 × 0,016 m (bredde × dybde × høyde). Blender-oppskrift og .blend ligger i `Unity/Blender`; FBX og kildebeskrivelse i `Unity/Assets/Signal47/Art/Archive16`. Unity beholder importrotasjonen under en plasseringsforelder og kontrollerer faktisk verdensgrense. Stolen og materialene gjenbrukes fra Chapter09; eksisterende lisenser følger pakken.

## Utført løkke og gjenstående porter

Første kjøring bestod 18 API-kontroller, men bildene viste for stor mappetekst, skapetiketter synlige gjennom bordet og for sterkt arbeidslys. Korrigeringen tilpasser etiketten til det importerte etikettfeltet, bruker en egen dybdetestet tekstshader med oppdatert fontatlas og demper arbeidslyset. Originalt førbilde beholdes som sammenligningsgrunnlag.

Blender-oppskriftens første renderforsøk manglet et world-objekt. Eksport og bounds-prøve var fullført, men dette var ikke et rendret resultat. Oppskriften ble rettet og kjørt med eksplisitt Python-feilkode før ny inspeksjon. Ingen manglende bildeproduksjon regnes som bestått.

Endelige resultater, identiteter og bildefiler ligger i [Evidence/verification.json](Evidence/verification.json). Konsept, Blender-render og Unity-capture er forskjellige beviskategorier. Ingen muse-/tastaturinput er sendt til det låste skrivebordet. Native interaksjon, fokus/Enter/Tab, ny ytelsesmåling, andre oppløsninger og ekstern forståelsestest er **UNVERIFIED**.

P05, historisk negativ, fastmerke, områdeovergang og nytt kapittel er ikke implementert. Det opprinnelige 45-minuttersanslaget for K2 og 5–6-timersmålet for hele spillet er fortsatt uprøvd; denne korte prøven dokumenterer ikke noen av varighetene.

## Reproduser

```bash
/home/tombonator3000t/signal47-tools/blender-4.5.13-linux-x64/blender --background --python-exit-code 2 --python Unity/Blender/Source/archive16_folio.py
bash Automation/build-archive16-linux.sh
bash Automation/run-archive16-checks.sh "$PWD/Artifacts/GauntletLinux/Signal47.x86_64"
python3 Automation/package-chapter09.py --label Archive16 --candidate --expect-source SHA256_FRA_MANIFEST
```

Blender-oppskriften lagrer kilde før renderoppsettet. Unity-scriptet genererer bare `Archive16_Study.unity`; hovedscenen og bakingen røres ikke. `Artifacts/GauntletLinux` er en muterbar byggmappe og inneholder etter dette Archive16, mens de navngitte releasepakkene beholdes. `Archive16` krever `--candidate` ved pakking og kan ikke stille erstatte standardstarteren. Kjøretesten krever en eksplisitt spillersti og oppretter sin egen merkede evidensmappe.

Neste steg: prøv dokumentflyten med faktisk input og en ny leser; noter hvilken begrunnelse vedkommende gir før støtteteksten vises. Bruk resultatet til å fastsette P04s endelige interaksjon og K2-budsjett før P05-negativ og full arkivsekvens produseres.

## Åpne prøvepakken

[Start Archive16](../../Artifacts/Releases/Archive16-4748e9d2fc14/Start-SIGNAL47.sh) eller pakk ut [Linux-arkivet](../../Artifacts/Releases/SIGNAL47-Archive16-4748e9d2fc14-Linux.tar.gz). Filene er lokale leveranser og følger ikke et vanlig Git-klon. Pakken starter direkte ved arkivbordet.

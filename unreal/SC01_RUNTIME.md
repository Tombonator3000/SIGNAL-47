# SC01: Den første overflygningen

Arbeidsgren: `codex/story-sc01`. Gjennomføringen følger [den vedtatte planen](HISTORIE_IMPLEMENTERINGSPLAN.md). Dokumentasjonen av hovedplanen er flettet gjennom PR166 i `5f14e4ce`.

## Omfang og sannhetsgrenser

Ny kohort1 eier SC01 og fortsetter deretter gjennom eksisterende historie. Gamle saker og eksisterende innholdsrevisjoner får ingen nye bakoverrettede oppgaver. H01 forblir åpent til SC02 er levert. Alle79 manus-ID-er er eksportert, men bare SC01 får nye verdensprodusenter i denne leveransen.

SC01 har fysisk normalrunde, kamera og faktisk instant-back-foto, kalibreringskvittering, begge eksisterende utganger, direkte/glasskontroll av referanselampen, opak skjerm, bevegelig original hånd utenfor glass, fysiske merker og en faktisk dispatchkontakt. Observasjoner følger kamera, sikt og verdenshandling; dialogpresentasjon gir ingen fysisk kontrollkvittering. Oversett hånd gir ingen påstand om sett kropp.

HorrorStory beholder sine kilder og alle 16 dokumenter. Det deterministiske SC-/H-registeret plasserer de seks tidligere ukoblede H01/H02-scenene. Senere SC-er er fortsatt produksjonsarbeid.

## Kontroller før pakket aksept

- Manusgenerator og 24 Python-kontrakter består.
- Editorbygg består etter retting av kompilatorfunn.
- Native import består for original hånd, glasspor og skjermspile, med eksakte trekanttall, mål, akser og kildehash.
- Fem faktiske SC01-takes er produsert og importert, 35,07 sekunder i 48 kHz PCM16 mono. Teknisk lyd/hash/timing består. Fire har eksakt ASR; SARO transkriberes som Sorrow i ett egennavn. Menneskelig lytting er UNVERIFIED.
- Editorens samlede selvtest består 1501 kontroller. Dette er ikke pakket aksept.

Første liveprøve fant manglende tastaturfokus i samtalepanelene; produksjonsknappene får nå fokus etter linjeskifte. Videre faktisk kjøring har bestått innsamling/kalibrering, begge native utganger, utvendig og gjennom-glass-lampetest, faktisk foto, skjerming, modalpause og bevegelse av hånden samt disk-/Flow-restores. Prøven fant deretter at sikting mot tomrom mellom smale fingermerker ikke kunne velge inspeksjonen. En avgrenset fysisk inspeksjonsflate er lagt til, med separat siktkrav til synlig originalgeometri; ny kjøring pågår.

Uavhengig kildegjennomgang fant og rettet gjenåpning av fullført SC01 ved senere døråpning, reserve-JPEG som falskt fotobevis, lydkvittering ved mute, senere lysoverskriving og ugyldige ventende scener. Manglende fotobevis eller ugyldig fysisk lagring beholder eksisterende saksprogresjon uten nye SC01-porter.

## Leveringsstatus

**Ikke levert.** Pakket High/Low-aksept, komplette ankomstruter med ny kohort, avgrenset regresjon, bilde-/RT-kontroll, runtime-PR, fletting og installasjon gjenstår.

Bevisrot: `/home/tom/scratch-bm/Signal47-sc01-20261026`. Tidlige FAIL-kjøringer beholdes. Shader-/pluginmeldinger i editor og editorens RT-varsel er ikke bevis for pakket ytelse eller minne. Menneskelig frykt, forståelse, lytting, fysisk Xbox og helnatt er fortsatt egne prøver.

## Overtatt kontroll 26. oktober 2026

Ny kjøring `editor-live-takeover-01` består etter filmrettingen: 120 kontroller, 17 disk-/Flow-restores og 139,99 m native gange. 35 eksplisitt syntetiske albumoppføringer er bare budsjettfixture; den siste referansen eksponeres faktisk gjennom vanlig Shutter. Feil motiv forbruker ikke reserven. Tidligere helt tom intern kohort beholder alle bilder og eksisterende progresjon uten oppdiktet referanse.

Alle fem faktiske SC01-takes har i editorprøven fullført ekte telefonruting med positive source-/routed-samples. Manusets 24 Python-kontrakter, ni eksportkontroller, stemmekatalogen og installerens 14 isolerte prøver består. Generert stemmemanifest er presisert slik at hele forfatterpakkens status ikke forveksles med de fem importerte SC01-takesene. Streng ASR har fortsatt ett SARO/Sorrow-egennavnavvik; menneskelig lytting er UNVERIFIED.

Dette ferdigmelder ingen pakket kandidat eller installasjon. Neste kontroll er samme kilderevisjon som pakket High/Low, usett-hånd-variant, begge komplette ankomstruter, fersk vanlig Continue og avgrenset regresjon.

## R1 pakket prøve og lokal RT-retting

R1 fra `f935b237` består den komplette pakkede SC01-prøven med 120 kontroller og 17 disk-/Flow-restores. Minnemålingen feiler separat: 80,7112 MiB AlwaysResident RT-geometri overstiger uendret grense på 80 MiB. R1 er derfor ikke leveringsakseptert. Byggekvitteringens første etterkontroll avviste Unreals to genererte FileOpenOrder-logger; disse er bevart og bare de opprinnelig rene kildeversjonene gjenopprettet. Full kontroll av alle 14 006 inputs og 40 uendrede pakkefiler består i `packaging-r1-complete`.

Native r2-import består alle rapporterte kontroller. Bare håndasseten er endret: egen redusert RT-proxy, full synlig kilde/fallback og kollisjon på 19 388 trekanter. Blender/FBX og de andre seks SC01-assetene er bytebevart. Ingen global terskel eller RT-innstilling endres. Testhjelperen forventet en verbose loggmelding som ikke skrives på commandletens displaynivå; denne FAIL-rapporten beholdes. Faktisk vellykket Python-commandlet, exitcode0 og strukturert native PASS er kontrollert separat i `native-import-r2-audit.json`. Ny pakket minne-/bildeprøve er nødvendig.

Bildedriveren venter nå på at merkene faktisk er tegnet før den etterfølgende se-bort-prøven. Dette retter tidsvalget for bevisbildet; inspeksjonsreglene er uendret.

# SIGNAL / 47 — aktuell overlevering

Oppdatert 12. september 2026 fra faktisk lokal Git-status, bygg og pakkekontroll. GitHub-hovedgrenen ble kontrollert på nytt; PR11 er flettet på `f2d92a7a8cc770849346221751a752831b6b2637`. Kanonisk mappe: `/home/tombonator3000t/.codex/.chatgpt-projects/g-p-6aa05604a4708191a65139c1d5f89363/SIGNAL-47`.

GitHub API bekrefter nå `private: false`, `visibility: public` for Tombonator3000/SIGNAL-47. Den eldre instruksens «privat repo» er dermed utdatert. Brukeren har uttrykkelig godkjent publisering av NightSky12-grenen og opprettelse av PR uten sin egen gjennomgang først. Brukeren har deretter selv flettet [PR8](https://github.com/Tombonator3000/SIGNAL-47/pull/8); merge er kontrollert via GitHub, commit `f156a9f2f656d168356145dabbe0bc8fb4ef0ec9`, 11. september 2026 kl. 10:32:08 UTC. Dette godkjenner ikke uutførte tester. Repoets synlighet er ikke endret av agenten.

## Gjeldende spill og ny kandidat

**Visual10 er den siste verifiserte spillutgaven.** PR7 ble merget 11. september; PR8 og PR9 er også flettet; PR9s mergecommit er `52c23abf28b3bf74adccb99d0f392fb8640892fb`. `./Spill-SIGNAL47.sh` starter fortsatt den faste pakken `Artifacts/Releases/Visual10-eb0748660027/`. Første kapittel har nattevakt, S-03, fysisk framkalling, bildeundersøkelse, aktiv/passiv B-12-prøve, to faktiske eksponeringer, lokal avslutning og lagring. Se [Visual10](VISUAL_10.md) og [Chapter09](CHAPTER_09.md).

**NightSky12 er en ny testkandidat**, med 8K katalogbasert stjernehimmel og dempet Melkevei, integrert i hovedgrenen gjennom PR8. Kildecommit `94811347a00811648f937400a1522c7d2cdd1d2b`. Unity er faktisk bygget; originalbilder fra faste inspeksjonskameraer er tatt i den utpakkede spilleren. Full brukerreise, lagring/fotobevis, bevegelse og ny ytelsesmåling er fortsatt UNVERIFIED fordi Kubuntu-skjermen var låst. Native tester oppdager nå låsen før de starter. Brukeren er bedt om å låse opp; låsen er ikke omgått.

Kandidaten åpnes med `Artifacts/Releases/NightSky12-79cafef6f5a5/Start-SIGNAL47.sh`. Flyttbar pakke: `Artifacts/Releases/SIGNAL47-NightSky12-79cafef6f5a5-Linux.tar.gz`. Den er merket TESTKANDIDAT. Kilde, dokumentasjon og bevisbilder er offentlig publisert og flettet gjennom PR8 av brukeren. Begge spillpakkene er lokale leveranser og følger ikke et rent Git-klon.

[NightSky12-rapporten](NIGHT_SKY_12.md) beskriver endringen, faktisk kjørte kontroller, skillet mellom bilder og spillerreise, og vurderingen av X-bildene / Magnific / Grok / Claude. Konsept11 «Den slettede natten» er bevart på `concept/story-sky-11` med åtte genererte bilder. Ny lore, himmelhendelser, motell og bil er fortsatt forslag eller senere innhold.

## Kildeidentitet og målinger

NightSky12: Unity SHA `79cafef6f5a59d042d101652cb5fb55942de4b13af91e8949a080d4caf580512`, byggpayload `c04306ebfdfe5046170ff419acb4281dfb5fcdc90137d4a853dfadb453c8b51a`, arkiv SHA `fc49644ab2a853d4afb2cf1e1bdc45425e3e3fa9d27039ff0260534cb379545d`. Senere dokumentasjonscommits endrer ikke denne Unity-kilden. [Innsjekket bevissett](Evidence/NightSky12/final-9481134/README.md) dokumenterer ni inspiserte sluttbilder, førbilde, korrigert polfeil, bygging, pakkeinnhold og den negative skjermlåstesten. Full arbeidslogg ligger lokalt i `Artifacts/Sky12/`.

Visual10: kildecommit `d026cc3`, Unity SHA `eb0748660027744a8ad717c16a51a1e8801f4908b758e4ca191e24108c9a2c01`. Historisk måling på Intel Core Ultra 5 225U / Mesa Intel ARL / OpenGLCore / 1280×800 Ultra / 75 Hz: 73,412 fps over 121,533 sekunder etter 20 sekunders oppvarming; p95 15,368 ms, p99 19,364 ms, maksimum 30,718 ms, ingen intervaller over 50 ms. Dette gjelder Visual10, ikke den nye himmelen. GPU-tid, subjektiv lydmiks og ekstern førstegangsspilltesting er fortsatt uverifisert.

## Designbibel 13 - nytt arbeidsgrunnlag

Brukeren ønsker nå en komplett historie og en tydelig ramme for et gjennomførbart spill på 5–6 timer. [Designbibel v0.1](DesignBible13/README.md) er et sammenhengende forslag: tre steder, seks kapitler og epilog, 18 hovedoppgaver, to avslutninger, 330 minutters førstegangsmål, kildetekster, fire kart, tre genererte konseptbilder og seks UI-skisser. PDF og redigerbare kilder er nå offentlig publisert og flettet gjennom PR9 etter brukerens «Fortsett, merge». Dokumentenes opprinnelige v0.1-status er historikk; selve historieforslaget innfører ingen nye Unity-områder.

Eksisterende kanon er skilt fra nye forslag. Nora/Tomás-forløpet, fenomenets presise lokale regler og sluttvalget må gjennomgås før ny historie implementeres. Tidsbudsjettet er ikke målt spilletid. De gamle punktene om en enkelt himmelhendelse er nå underlagt den samlede foreslåtte beviskjeden og roadmappen; ikke bygg en løs hendelse som motsier denne rammen.

## Menu14 — første implementeringssteg etter bibelen

Brukeren ba 12. september om å fortsette. Designbibelens trestedsramme brukes som arbeidsretning; dette passet innfører ingen ny lore i Unity. [Menu14](Menu14/README.md) retter eksisterende start-/checkpoint-flyt: Fortsett først når en sak finnes, eksplisitt avbrytbar ny-sak-bekreftelse, bevaring av begge checkpoint-filer i et unikt PreviousCases-arkiv og ingen overgang ved kopieringsfeil. Originalfoto bevares. Ingen tre-saks-/flerplassmeny eller nye historieområder er implementert.

[P04/P05-papirprøven](Menu14/PAPER_CASE.md) har kildekort, diagram, fasit og dokumentert egenkontroll. Vedlikeholdskortets for tidlige avsløring er rettet i prøveteksten. Dette er ikke en bestått blindtest; M1 og 5–6-timersmålet er fortsatt åpne. Den eldre designbibel-PDF-en er bevart som v0.1.

28 API-styrte kontroller består i faktisk Unity-spiller og gjennom launcher etter utpakking. De inkluderer feil, avbryt, faktisk scenelasting av en tidligere fullført v1-sak og dekoding av to originale foto med identiske bytes. Originale menyskjermer er inspisert. Ingen nye native tastatur-/musforsøk eller ytelsesmålinger er utført. Menu14 er en separat testkandidat; Visual10 er fortsatt standardstarteren. Se [verifikasjon og pakkeidentitet](Menu14/Evidence/verification.json). Arbeidet ble flettet som [PR9](https://github.com/Tombonator3000/SIGNAL-47/pull/9), mergecommit `52c23abf28b3bf74adccb99d0f392fb8640892fb`, 12. september kl. 18:24:43 UTC. GitHub bekreftet MERGED; vanlig merge uten bypass. Dette endrer ikke testkandidatstatus eller standardstarter.

## Recovery15 — videreført tilgang til tidligere saker

[Recovery15](Recovery15/README.md) bygger videre på PR9: Previous Shifts i startmenyen, liste med lokal dato og backup-status, avbrytbar forhåndsvisning, ny validering før bytte og bevaring av gjeldende sak før gjenoppretting. Samme v1-format og fotofiler brukes. Dette er en avgrenset del av M2, ikke ferdig treplasslagring, områdeovergang eller ny historie. Unity-bygg og 50 API-kontroller består både direkte og gjennom launcher fra utpakket kandidat. Begge originale foto og kildeprofilen er uendret; seks originale menystater er visuelt inspisert. Kandidat: `Recovery15-bda088dfe638`. Native input og ytelse er fortsatt UNVERIFIED; Visual10 er standard. Bygg-, pakke- og testidentitet følger [verifikasjonsrapporten](Recovery15/Evidence/verification.json). Kilde og bevis ble flettet som [PR10](https://github.com/Tombonator3000/SIGNAL-47/pull/10), commit `51752367a591ba6a900b7006e6efa8a1af5813e4`, 12. september kl. 18:45:30 UTC; MERGED er kontrollert via GitHub. Brukerens «Fortsett, merge» autoriserer vanlig fletting av denne avgrensede videreføringen; testportene over gjelder uavhengig av flettestatus.

## Archive16 — selvstendig arkivprøve

Brukeren ba deretter om å fortsette med gauntlet-loop og game-production, etter forslaget om en liten visuell arkivprøve. [Archive16](Archive16/README.md) prøver P04 ved et fysisk bord: åpne mappe, les original og korrigert protokoll, sammenlign og velg en støttet slutning. Egen Unity-scene med to faste inspeksjonskameraer; ingen ny gangrute, kapittelovergang eller lagring. Dette er en separat produksjonsprøve og lukker ikke M1/M2 eller K2s spilletidsbudsjett.

Blender-oppskrift, redigerbar mappe og FBX er bevart; Unity-bounds er kontrollert. Første visuelle inspeksjon fant for stor tekst, etiketter gjennom bordet og for sterkt lys; disse er korrigert. Alle 18 API-kontroller består både direkte og gjennom en nyutpakket launcher; åtte originale runtime-bilder er inspisert. Kandidat `Archive16-4748e9d2fc14` og endelig testidentitet følger [verifikasjonsrapporten](Archive16/Evidence/verification.json). Kilde og bevis er publisert i [PR11](https://github.com/Tombonator3000/SIGNAL-47/pull/11) på `gauntlet/archive-16`, nå flettet; MERGED og mergecommit `f2d92a7a8cc770849346221751a752831b6b2637` er kontrollert via GitHub. `Artifacts/GauntletLinux` inneholder nå arkivprøven; navngitte hovedspillpakker og `Spill-SIGNAL47.sh` er bevart.

## Archive17 — Blender-pass

Brukerens neste bestilling var bedre Blender-modeller. [Archive17](Archive17/README.md) erstatter bord, skap, lampe, mappe og papirbunke i den selvstendige arkivprøven med fem originale FBX-modeller og redigerbar `.blend`-kilde. Samme P04-handling, kameraer og kildetekster. Sammenfallende bordflater og synlig lysrør fra siden ble oppdaget og rettet i faktisk visuell kontroll. UV-er, normaler, dimensjoner og geometribudsjett er kontrollert etter Unity-import. Ingen Magnific-generering var nødvendig; bilde-til-3D-verktøyet finnes i den tilkoblede katalogen.

Ny lokal kandidat: `Artifacts/Releases/Archive16-74eb85f963c7/Start-SIGNAL47.sh` (Archive16-familie, Archive17-modeller). Kilde-SHA `74eb85f963c76cbb6f5277e3605ce68d1c843da97e7f1d74460c857c21d89082`. Blender- og Unity-bilder har separate merkinger. Se rapporten for tester, pakkeidentitet og gjenværende porter. Visual10 er fortsatt standard. Hovedspillets scene og lagring er bevart. Kilde og bevis er pushet som [PR12](https://github.com/Tombonator3000/SIGNAL-47/pull/12) på `gauntlet/archive-models-17`; åpen ved denne overleveringen. 18 API-kontroller består direkte og fra utpakket launcher; åtte sluttbilder er inspisert. Native input og releaseytelse er fortsatt UNVERIFIED.

## Fortsett her

Brukeren opplyste 11. september at vedkommende er borte hjemmefra og ikke kan låse opp skjermen nå. Ikke gjenta opplåsingsforespørselen eller start flere native inputforsøk mens dette gjelder. Bygging, statisk runtime-inspeksjon og pakkekontroll er allerede utført; manglende interaktive kontroller utsettes til skrivebordet er tilgjengelig. Ingen overvåking eller bakgrunnsjobb er startet.

1. Prøv Archive16 med ekte input og en ny leser når dette er mulig; registrer begrunnelsen før fasit vises. Bruk også P04/P05-spillerarket i Menu14 til den komplette papirprøven. Dokumenter begrunnelser og tidsbruk; egenkontrollen er allerede gjort. Før større historieproduksjon må åpne kanonvalg og innholdstak fryses.
2. Når brukeren har tilgang til et opplåst Kubuntu-skrivebord: kontroller Recovery15s faktiske knapper, Escape og tastaturfokus, listesider og valg av tidligere sak, avbryt fra meny/pause, Fortsett etter prosessgjenstart og begge forsøksveier. Recovery15 inkluderer Menu14 og NightSky12; en full ny tur på samme pakke kan dekke begge. Bruk nye, isolerte testprofiler.
3. Kontroller de virkelige fotografiene, inkludert gjenåpning, og himmelen under vanlig gange og raske vendinger. Kjør separat releaseytelse med alle frameintervaller beholdt.
4. Korriger eventuelle funn før kandidaten kan erstatte Visual10. Følg deretter M1–M3 i designbibelens roadmap: papirprøve, trygg lagring/områdeovergang og kort spillbar arkivsekvens. Ikke start alle nye systemer og miljøer samtidig.

Bygg himmelkandidaten med `bash Automation/build-night-sky12-linux.sh`; dette bevarer de to eksisterende lablysatlasene. `build-visual10-linux.sh` regenererer og baker scenen på nytt, nå også med himmelpasset. Eldre genereringsskript er ikke oppskriften for et identisk bakt releasebygg.

Behold Unity 6000.3.22f1 / URP 17.3.0 / Input System 1.20.0 / Blender 4.5.13, originalmodeller og metadata, NASA-kildespor og øvrige lisenser, VT323/Scott Buckley og 1419.900 / 1420.110 / 1420.405 MHz, 4/7, −39 LY og 47 spillsekunder. Ingen brukerlagring, synkronisert `sources/` eller andre prosjekter er endret. Ingen ny bakgrunnsjobb er planlagt etter leveringen.

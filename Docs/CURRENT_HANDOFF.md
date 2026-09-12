# SIGNAL / 47 — aktuell overlevering

Oppdatert 12. september 2026 fra faktisk lokal Git-status, bygg og pakkekontroll. GitHub-hovedgrenen ble kontrollert på nytt og er uendret. Kanonisk mappe: `/home/tombonator3000t/.codex/.chatgpt-projects/g-p-6aa05604a4708191a65139c1d5f89363/SIGNAL-47`.

GitHub API bekrefter nå `private: false`, `visibility: public` for Tombonator3000/SIGNAL-47. Den eldre instruksens «privat repo» er dermed utdatert. Brukeren har uttrykkelig godkjent publisering av NightSky12-grenen og opprettelse av PR uten sin egen gjennomgang først. Brukeren har deretter selv flettet [PR8](https://github.com/Tombonator3000/SIGNAL-47/pull/8); merge er kontrollert via GitHub, commit `f156a9f2f656d168356145dabbe0bc8fb4ef0ec9`, 11. september 2026 kl. 10:32:08 UTC. Dette godkjenner ikke uutførte tester. Repoets synlighet er ikke endret av agenten.

## Gjeldende spill og ny kandidat

**Visual10 er den siste verifiserte spillutgaven.** PR7 ble merget 11. september; PR8 er nå også flettet, og hovedgrenen er `f156a9f2f656d168356145dabbe0bc8fb4ef0ec9`. `./Spill-SIGNAL47.sh` starter fortsatt den faste pakken `Artifacts/Releases/Visual10-eb0748660027/`. Første kapittel har nattevakt, S-03, fysisk framkalling, bildeundersøkelse, aktiv/passiv B-12-prøve, to faktiske eksponeringer, lokal avslutning og lagring. Se [Visual10](VISUAL_10.md) og [Chapter09](CHAPTER_09.md).

**NightSky12 er en ny testkandidat**, med 8K katalogbasert stjernehimmel og dempet Melkevei, integrert i hovedgrenen gjennom PR8. Kildecommit `94811347a00811648f937400a1522c7d2cdd1d2b`. Unity er faktisk bygget; originalbilder fra faste inspeksjonskameraer er tatt i den utpakkede spilleren. Full brukerreise, lagring/fotobevis, bevegelse og ny ytelsesmåling er fortsatt UNVERIFIED fordi Kubuntu-skjermen var låst. Native tester oppdager nå låsen før de starter. Brukeren er bedt om å låse opp; låsen er ikke omgått.

Kandidaten åpnes med `Artifacts/Releases/NightSky12-79cafef6f5a5/Start-SIGNAL47.sh`. Flyttbar pakke: `Artifacts/Releases/SIGNAL47-NightSky12-79cafef6f5a5-Linux.tar.gz`. Den er merket TESTKANDIDAT. Kilde, dokumentasjon og bevisbilder er offentlig publisert og flettet gjennom PR8 av brukeren. Begge spillpakkene er lokale leveranser og følger ikke et rent Git-klon.

[NightSky12-rapporten](NIGHT_SKY_12.md) beskriver endringen, faktisk kjørte kontroller, skillet mellom bilder og spillerreise, og vurderingen av X-bildene / Magnific / Grok / Claude. Konsept11 «Den slettede natten» er bevart på `concept/story-sky-11` med åtte genererte bilder. Ny lore, himmelhendelser, motell og bil er fortsatt forslag eller senere innhold.

## Kildeidentitet og målinger

NightSky12: Unity SHA `79cafef6f5a59d042d101652cb5fb55942de4b13af91e8949a080d4caf580512`, byggpayload `c04306ebfdfe5046170ff419acb4281dfb5fcdc90137d4a853dfadb453c8b51a`, arkiv SHA `fc49644ab2a853d4afb2cf1e1bdc45425e3e3fa9d27039ff0260534cb379545d`. Senere dokumentasjonscommits endrer ikke denne Unity-kilden. [Innsjekket bevissett](Evidence/NightSky12/final-9481134/README.md) dokumenterer ni inspiserte sluttbilder, førbilde, korrigert polfeil, bygging, pakkeinnhold og den negative skjermlåstesten. Full arbeidslogg ligger lokalt i `Artifacts/Sky12/`.

Visual10: kildecommit `d026cc3`, Unity SHA `eb0748660027744a8ad717c16a51a1e8801f4908b758e4ca191e24108c9a2c01`. Historisk måling på Intel Core Ultra 5 225U / Mesa Intel ARL / OpenGLCore / 1280×800 Ultra / 75 Hz: 73,412 fps over 121,533 sekunder etter 20 sekunders oppvarming; p95 15,368 ms, p99 19,364 ms, maksimum 30,718 ms, ingen intervaller over 50 ms. Dette gjelder Visual10, ikke den nye himmelen. GPU-tid, subjektiv lydmiks og ekstern førstegangsspilltesting er fortsatt uverifisert.

## Designbibel 13 - nytt arbeidsgrunnlag

Brukeren ønsker nå en komplett historie og en tydelig ramme for et gjennomførbart spill på 5–6 timer. [Designbibel v0.1](DesignBible13/README.md) er et sammenhengende forslag: tre steder, seks kapitler og epilog, 18 hovedoppgaver, to avslutninger, 330 minutters førstegangsmål, kildetekster, fire kart, tre genererte konseptbilder og seks UI-skisser. PDF og redigerbare kilder følger samme lokale gren `design/bible-13`. Ingen Unity-kode er endret og ingen ny PR/push er utført for historieforslaget.

Eksisterende kanon er skilt fra nye forslag. Nora/Tomás-forløpet, fenomenets presise lokale regler og sluttvalget må gjennomgås før ny historie implementeres. Tidsbudsjettet er ikke målt spilletid. De gamle punktene om en enkelt himmelhendelse er nå underlagt den samlede foreslåtte beviskjeden og roadmappen; ikke bygg en løs hendelse som motsier denne rammen.

## Menu14 — første implementeringssteg etter bibelen

Brukeren ba 12. september om å fortsette. Designbibelens trestedsramme brukes som arbeidsretning; dette passet innfører ingen ny lore i Unity. [Menu14](Menu14/README.md) retter eksisterende start-/checkpoint-flyt: Fortsett først når en sak finnes, eksplisitt avbrytbar ny-sak-bekreftelse, bevaring av begge checkpoint-filer i et unikt PreviousCases-arkiv og ingen overgang ved kopieringsfeil. Originalfoto bevares. Ingen tre-saks-/flerplassmeny eller nye historieområder er implementert.

[P04/P05-papirprøven](Menu14/PAPER_CASE.md) har kildekort, diagram, fasit og dokumentert egenkontroll. Vedlikeholdskortets for tidlige avsløring er rettet i prøveteksten. Dette er ikke en bestått blindtest; M1 og 5–6-timersmålet er fortsatt åpne. Den eldre designbibel-PDF-en er bevart som v0.1.

28 API-styrte kontroller består i faktisk Unity-spiller og gjennom launcher etter utpakking. De inkluderer feil, avbryt, faktisk scenelasting av en tidligere fullført v1-sak og dekoding av to originale foto med identiske bytes. Originale menyskjermer er inspisert. Ingen nye native tastatur-/musforsøk eller ytelsesmålinger er utført. Menu14 er en separat testkandidat; Visual10 er fortsatt standardstarteren. Se [verifikasjon og pakkeidentitet](Menu14/Evidence/verification.json). Arbeidet ligger lokalt på `gauntlet/menu-14`; ingen ny push, PR eller merge er gjort i dette passet.

## Fortsett her

Brukeren opplyste 11. september at vedkommende er borte hjemmefra og ikke kan låse opp skjermen nå. Ikke gjenta opplåsingsforespørselen eller start flere native inputforsøk mens dette gjelder. Bygging, statisk runtime-inspeksjon og pakkekontroll er allerede utført; manglende interaktive kontroller utsettes til skrivebordet er tilgjengelig. Ingen overvåking eller bakgrunnsjobb er startet.

1. Bruk det ferdige P04/P05-spillerarket i Menu14 til en ny lesers papirprøve. Dokumenter begrunnelser og tidsbruk; egenkontrollen er allerede gjort. Før større historieproduksjon må åpne kanonvalg og innholdstak fryses.
2. Når brukeren har tilgang til et opplåst Kubuntu-skrivebord: kontroller Menu14s faktiske knapper, Escape og tastaturfokus, avbryt fra meny/pause, Fortsett etter prosessgjenstart og begge forsøksveier. Menu14 inkluderer NightSky12; en full ny tur på samme pakke kan dekke begge. Bruk nye, isolerte testprofiler.
3. Kontroller de virkelige fotografiene, inkludert gjenåpning, og himmelen under vanlig gange og raske vendinger. Kjør separat releaseytelse med alle frameintervaller beholdt.
4. Korriger eventuelle funn før kandidaten kan erstatte Visual10. Følg deretter M1–M3 i designbibelens roadmap: papirprøve, trygg lagring/områdeovergang og kort spillbar arkivsekvens. Ikke start alle nye systemer og miljøer samtidig.

Bygg himmelkandidaten med `bash Automation/build-night-sky12-linux.sh`; dette bevarer de to eksisterende lablysatlasene. `build-visual10-linux.sh` regenererer og baker scenen på nytt, nå også med himmelpasset. Eldre genereringsskript er ikke oppskriften for et identisk bakt releasebygg.

Behold Unity 6000.3.22f1 / URP 17.3.0 / Input System 1.20.0 / Blender 4.5.13, originalmodeller og metadata, NASA-kildespor og øvrige lisenser, VT323/Scott Buckley og 1419.900 / 1420.110 / 1420.405 MHz, 4/7, −39 LY og 47 spillsekunder. Ingen brukerlagring, synkronisert `sources/` eller andre prosjekter er endret. Ingen ny bakgrunnsjobb er planlagt etter leveringen.

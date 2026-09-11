# SIGNAL / 47 — aktuell overlevering

Oppdatert 11. september 2026 fra faktisk lokal Git-status og GitHub. Kanonisk mappe: `/home/tombonator3000t/.codex/.chatgpt-projects/g-p-6aa05604a4708191a65139c1d5f89363/SIGNAL-47`.

GitHub API bekrefter nå `private: false`, `visibility: public` for Tombonator3000/SIGNAL-47. Den eldre instruksens «privat repo» er dermed utdatert. NightSky12-grenen er **kun lokal**; ingen push/PR er utført fordi nye filer da ville blitt offentlig publisert. Ikke endre synlighet eller publiser kandidaten uten relevant autorisasjon.

## Gjeldende spill og ny kandidat

**Visual10 er den siste verifiserte spillutgaven.** PR7 ble merget 11. september; hovedgrenen er `1db7b41e99cf8cfe5d7ca31fb12d3782c0f18cc4`. `./Spill-SIGNAL47.sh` starter fortsatt den faste pakken `Artifacts/Releases/Visual10-eb0748660027/`. Første kapittel har nattevakt, S-03, fysisk framkalling, bildeundersøkelse, aktiv/passiv B-12-prøve, to faktiske eksponeringer, lokal avslutning og lagring. Se [Visual10](VISUAL_10.md) og [Chapter09](CHAPTER_09.md).

**NightSky12 er en ny testkandidat**, med 8K katalogbasert stjernehimmel og dempet Melkevei, på `gauntlet/night-sky-12`. Kildecommit `94811347a00811648f937400a1522c7d2cdd1d2b`. Unity er faktisk bygget; originalbilder fra faste inspeksjonskameraer er tatt i den utpakkede spilleren. Full brukerreise, lagring/fotobevis, bevegelse og ny ytelsesmåling er fortsatt UNVERIFIED fordi Kubuntu-skjermen var låst. Native tester oppdager nå låsen før de starter. Brukeren er bedt om å låse opp; låsen er ikke omgått.

Kandidaten åpnes med `Artifacts/Releases/NightSky12-79cafef6f5a5/Start-SIGNAL47.sh`. Flyttbar pakke: `Artifacts/Releases/SIGNAL47-NightSky12-79cafef6f5a5-Linux.tar.gz`. Den er merket TESTKANDIDAT. Ingen ny merge eller offentlig publisering er utført. Begge spillpakkene er lokale leveranser og følger ikke et rent Git-klon.

[NightSky12-rapporten](NIGHT_SKY_12.md) beskriver endringen, faktisk kjørte kontroller, skillet mellom bilder og spillerreise, og vurderingen av X-bildene / Magnific / Grok / Claude. Konsept11 «Den slettede natten» er bevart på `concept/story-sky-11` med åtte genererte bilder. Ny lore, himmelhendelser, motell og bil er fortsatt forslag eller senere innhold.

## Kildeidentitet og målinger

NightSky12: Unity SHA `79cafef6f5a59d042d101652cb5fb55942de4b13af91e8949a080d4caf580512`, byggpayload `c04306ebfdfe5046170ff419acb4281dfb5fcdc90137d4a853dfadb453c8b51a`, arkiv SHA `fc49644ab2a853d4afb2cf1e1bdc45425e3e3fa9d27039ff0260534cb379545d`. Senere dokumentasjonscommits endrer ikke denne Unity-kilden. [Innsjekket bevissett](Evidence/NightSky12/final-9481134/README.md) dokumenterer ni inspiserte sluttbilder, førbilde, korrigert polfeil, bygging, pakkeinnhold og den negative skjermlåstesten. Full arbeidslogg ligger lokalt i `Artifacts/Sky12/`.

Visual10: kildecommit `d026cc3`, Unity SHA `eb0748660027744a8ad717c16a51a1e8801f4908b758e4ca191e24108c9a2c01`. Historisk måling på Intel Core Ultra 5 225U / Mesa Intel ARL / OpenGLCore / 1280×800 Ultra / 75 Hz: 73,412 fps over 121,533 sekunder etter 20 sekunders oppvarming; p95 15,368 ms, p99 19,364 ms, maksimum 30,718 ms, ingen intervaller over 50 ms. Dette gjelder Visual10, ikke den nye himmelen. GPU-tid, subjektiv lydmiks og ekstern førstegangsspilltesting er fortsatt uverifisert.

## Fortsett her

1. Lås opp Kubuntu. Fullfør en fersk tastatur-/musreise på den eksakte NightSky12-pakken, begge forsøksveier og faktiske prosessgjenstarter. Bruk nye, isolerte testprofiler.
2. Kontroller de virkelige fotografiene, inkludert gjenåpning, og himmelen under vanlig gange og raske vendinger. Kjør separat releaseytelse med alle frameintervaller beholdt.
3. Korriger eventuelle funn før kandidaten kan erstatte Visual10. Deretter avgrenses én undersøkelig himmelhendelse fra konseptforslaget.

Bygg himmelkandidaten med `bash Automation/build-night-sky12-linux.sh`; dette bevarer de to eksisterende lablysatlasene. `build-visual10-linux.sh` regenererer og baker scenen på nytt, nå også med himmelpasset. Eldre genereringsskript er ikke oppskriften for et identisk bakt releasebygg.

Behold Unity 6000.3.22f1 / URP 17.3.0 / Input System 1.20.0 / Blender 4.5.13, originalmodeller og metadata, NASA-kildespor og øvrige lisenser, VT323/Scott Buckley og 1419.900 / 1420.110 / 1420.405 MHz, 4/7, −39 LY og 47 spillsekunder. Ingen brukerlagring, synkronisert `sources/` eller andre prosjekter er endret. Ingen ny bakgrunnsjobb er planlagt etter leveringen.

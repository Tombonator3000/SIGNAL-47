# Diner og Vulkan-minne, 29. oktober 2026

**Status:** Målrettet Linux-kandidat PASS; historisk restårsak åpen. **Gjelder:** FIX-01, UE 5.8.4 CL58902052, Linux/Vulkan, RTX 3080 10 GiB. **Sist kontrollert:** 29. oktober 2026.

Linux-kilden `b70bf042b841640b0121f7e00d20a2565cd3da37` reduserer den konservative reserveringen til VSMs synlige ikke-Nanite-instanser til en firedel. Den byteidentiske gamle krasjpakken fullførte diner og hele returen med dette ene tiltaket. Ny pakket kandidat R1 og kald High/nattkontroll R2 består uten diagnoseoverstyring gjennom [PR215](https://github.com/Tombonator3000/SIGNAL-47/pull/215). Ingen installasjon eller offentlig release.

## Bekreftet krasjgrunnlag

[Originalrapporten](../../Active/Reports/PLAYTEST_TODO_2026-10-28.md#2-diner-to-krasj-i-installert-spill) og [maskinlesbar triage](../../../unreal/Evidence/Playtest-2026-10-28/diner-triage-20261029.json) binder de komplette UE-loggene, kjernefilene og symbolene til riktig kjørbar fil. Begge krasj var Vulkan-OOM, ved henholdsvis 128 og 256 MiB transient heap. SIGSEGV fulgte motorens bevisste fatalavslutning.

Det første var High/frame0 etter LoadMap, uten bevist dinerhandling. Det andre var Ultra/TSR 200 etter waitress/counter/closer-look og hadde bare 117,25 MiB differanse mellom Vulkan EXT usage og budget før forespørselen på 256 MiB. Første budsjettpar forklarer ikke avslaget alene. Historisk samtidig editor eller annen bestemt prosess er fortsatt **UNVERIFIED**.

## Avgrensede A/B-prøver

Alle bruker den samme bevarte andre krasjpakken, faktisk 2560×1440, Ultra, Vulkan, isolerte `/tmp`-profiler og den fysiske `water`-ruten. Guard viste ingen andre tunge jobber. En overvåker avsluttet bare egen spillprosess ved under 384 MiB globalt ledig GPU-minne. Avbrutte prøver er sensurerte minnemålinger, ikke nye krasj eller fullført ruteaksept.

| Prøve | Eneste endring fra gammel Ultra | Global topp / minst ledig MiB | Resultat |
| --- | --- | --- | --- |
| R1 | Ingen | 9676 / 180 | Kontrollert stopp på veien før diner. |
| R2 | TSR-historikk 100 | 9533 / 322 | Kontrollert stopp før diner. Utilstrekkelig tiltak. |
| R3 | Nanite kandidater 4 Mi / synlige 1 Mi, sen innstilling | 9709 / 146 | Diner bestått, kontrollert stopp på vei ut. Utilstrekkelig tiltak. |
| R4 | Samme Nanite-kapasitet før RHI, faktisk 32/12 MiB fra første renderframe | 9593 / 262 | Diner bestått, kontrollert stopp før full retur. Utilstrekkelig tiltak. |
| R5 | Bare tidlig VSM-reservefaktor 0,25 | 8089 / 1767 | Hele diner/retur/lagring PASS, 311,5 rutesekunder og 2628 kjørte meter. |

R5 beholdt original Ultra/TSR 200 og Nanite 16 Mi/4 Mi. Vulkan device-local peak var 6385,99 MiB. Telefon, fullworld/diskrestore, D01, tre papirer og monoton klokke bestod. Alle sju gamle payloadfiler er hashkontrollert uendret etter R5. Startup-minnedumpen og andre diagnoseutskrifter påvirker veggklokken; tallene gir ingen FPS-aksept. Global `nvidia-smi`, Vulkan-allokering, EXT-budsjett og logisk RHI-bufferstørrelse er forskjellige målinger.

## Hvorfor denne endringen

Lokal primærkilde `Engine/Source/Runtime/Renderer/Private/VirtualShadowMaps/VirtualShadowMapArray.cpp`, rundt linje 3964, reserverer 12 byte ganger et konservativt antall instanser, primærvisninger og mipnivåer. Faktoren er opprinnelig 1. Disse varierende `Shadow.Virtual.VisibleInstances`-bufferne går i den vanlige RDG-bufferpoolen. En køet opplasting på null byte hindrer transientbanen; størrelser avrundes til 64 KiB og gamle størrelser beholdes over flere bilder. Dette forklarer målt bufferpress, men knytter ikke det historiske siste allokeringsavslaget til én bestemt buffer.

[LinuxEngine.ini](../../../unreal/Signal47/Config/Linux/LinuxEngine.ini) setter bare `r.Shadow.Virtual.NonNanite.CulledInstanceAllocationFactor=0.25`. Den endrer reserveringskapasitet. Geometri og skyggetegning beholdes så lenge kapasiteten ikke overskrides. Nanite, TSR, motor, driver og grafikkpresets er uendret. [Epics VSM-dokumentasjon](https://dev.epicgames.com/documentation/unreal-engine/virtual-shadow-maps-in-unreal-engine) beskriver de konservative Total-/Drawn-tellerne for ikke-Nanite-geometri.

R5 gav 300 separate GPU `StatusFeedback` type1-meldinger: null flaggbit8 for synlige instanser, seks flaggbit1 for marking-køen og ingen avkortede meldinger. Hele loggen mangler varsel om synlig-instansoverløp, allokeringsfeil og fatal. Marking-køflaggene er ikke hevdet rettet.

CSV-feltet `OVERFLOW_FLAGS` brukes **ikke** som aksept: UE5.8s `VirtualShadowMapPrintStats.usf` setter inn to clusterfelt uten tilsvarende indekskompensasjon senere i payloaden. Den separate statusmeldingen `[1, flags]` har riktig binding til overløpsbitene. Total/Drawn-feltene før forskyvningen er nyttige for reservemargin, men asynkrone tellere er ikke bevis på identisk CPU-/GPU-frame.

## Kandidat og grenser

Ny Linux-pakke R1 er bygget fra de 665 frosne inputfilene over, med Claude PR212 til PR214 integrert. Alle åtte pakkefiler og alle 665 inputfiler er hashkontrollert uendret etter ruten. Binærens SHA256 er `41c2441a9347bbbc8875e4b6c6cbb28b35c5df8bb398d025f77f295c0a46607d`. Faktisk konfigurasjonsreadback ved frame 0 er `0.25 LastSetBy: SystemSettingsIni`; kommandoen inneholder ingen `ForceDPCVars` eller setter for verdien. Ultra viser faktisk skalering 100, skygger 3, GI 3, refleksjon 2, HWRT 1, TSR 200 og Nanite-standardene 16 Mi/4 Mi.

**Pakket diner R1 PASS:** 1709 selvtester, fysisk diner og 2627,8 meter retur, 311,8 rutesekunder, telefon/fullworld-diskrestore, D01, papirer og monoton klokke. Global topp 8115MiB/minst1740MiBledig, Vulkan device-local peak 6451,62 MiB. 360 separate GPU-statusmeldinger har null synlig-instansbit8; fire marking-køflagg beholdes som avgrensning. Ingen fatal, allokeringsfeil eller meldingsavkorting. Alle fem faktiske PNG-er er 2560×1440; dinerbildet er åpnet. Diagnostikk påvirker veggtiden 433,47s, så dette er ikke FPS-aksept.

**Kald High/natt R2 PASS:** ordinær Continue fra en uttrykkelig isolert kopi av faktisk nattjournallagring ved trafoen kl. 01:35, før T01. Bare kameraretningen var endret i den tidligere kamerafixturen. Output 2560×1440, normal High-internskalering 60prosent. 3159 rå GPU-statusmeldinger etter `opened continue` har bare nullflagg; hele økten har null synlig-instansoverløp og ingen allokeringsfeil/fatal. Global topp 5694MiB/minst4161MiBledig, Vulkan device-local peak 4033,38 MiB. Bildet er åpnet og viser to amberlys og den rolige øvre effekten. Ingen fysisk feltvandring eller menneskelig input hevdes.

Første High/natt R1 er bevart som kaldstart og kort bilde, men avvist som natt-GPU-bevis: minnedumpen brukte målevinduet og de 60 statusmeldingene kom før Continue. R2 fjernet dumpen og målte gjennom hele nattvisningen. Dette er én begrunnet omprøve av målevinduet, med uendret kandidat/kilde.

Main/PR216 med TB2-bygningene landet etter b70bf042-frysen. Den er kildeflettet, men er **ikke** med i akkurat R1-pakken. Neste sammensatte motelpakke har egen kontroll; disse resultatene utvides ikke automatisk til den.

Etter lest tørrplan og guard er én eldre testpakke pensjonert, sju payloadfiler/4 448 643 003 logiske byte. Ny R1, natt-R2, natt-R1 og den nødvendige gamle krasjkomparatoren beholdes. Ny kandidat, komparator og installert payload er hashkontrollert uendret. Saved og Games var ikke i slettelisten. En første Saved-sammenligning brukte en utdatert installasjonsreferanse fra før Toms senere spilløkt; alle observerte avvik har fil-/foreldremappetid før nattarbeidet startet 22:58. Ingen nåværende lagring er gjenopprettet eller endret, og det hevdes ikke en ny full byte-for-byte Saved-før/etterkontroll uten et ferskt førbilde. Begge kontrollkvitteringer er bevart.

Første historiske oppstarts-OOM og historisk prosesskonkurranse forblir åpne undersøkelsespunkter. Ingen fullmatrise, menneskelig input, subjektiv lytting, Windows-/annen GPU-aksept eller generell skyggegaranti er utført her. Testregelen er én målrettet endret rute og konkrete tillegg bare når en ny relevant risiko avdekkes.

Private råbevis: `~/scratch-bm/Signal47-diner-crash-20261029`, med opprinnelige krasj, kjernefiler, kernelutdrag, guardkvitteringer, innstillinger, CSV, PNG og før-/etterhash. Første komplette pakke mangler; første exe/debug/sym er sikret. Den eneste komplette andre krasjpakken er midlertidig eksplisitt beskyttet mot opprydding som nødvendig komparator. Originalvedlegg er uendret.

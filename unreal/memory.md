## Fotolab levert, 28. oktober 2026 kl.01:15 CET

Fotolab R1 er flettet via [PR181](https://github.com/Tombonator3000/SIGNAL-47/pull/181), merge `8c70ecb69f24bae15197e31bab95c924e1fd18e5`, og identisk testet pakke er installert i `/home/tom/Games/Signal47/Linux`. Ti originale detaljdeler, PBR, tre LOD og redigerbar Blender-kilde følger med. Hele matrisen består 110/110 etter én kjøring. Installert Low-fremkalling og ny prosess med Continue fra faktisk disk består; begge bilder er vurdert. Alle 50 personlige Saved-filer er uendret. Motel R4, SC01, MetaHuman og godkjent atmosfære er bevart. Se [fotolabrapport](PHOTOLAB_DETAIL.md). Claudes fem TB2-bygninger i PR179 er fortsatt separate kilder uten native integrasjon i denne leveransen.

# Minne for unreal/

## Gjeldende levering, 27. oktober 2026 kl. 23:19 CET

Motel V2 er flettet til main via [PR #178](https://github.com/Tombonator3000/SIGNAL-47/pull/178), merge `64e2a1ae4a692212f72c36c7449c0027cb81a48d`, og samme testede R4 er installert i `/home/tom/Games/Signal47/Linux`. PR #172 er også flettet gjennom denne historikken. Standardversjonen kombinerer SC01, MetaHuman, atmosfære og det nye motellet.

Hele `screenshots.sh` består 110/110. Den installerte Low-prøven består 17 kontroller, inklusive tre faktiske taleklipp, pause og diskrestore. Bildet etter restore er åpnet og vurdert. Alle 50 personlige Saved-filer er uendret fra oppdragets start. Dette er automatiserte kontroller og bildevurdering, ikke menneskelig gjennomspilling eller en helspill-/FPS-garanti. Se [motelrapporten](MOTEL_V2.md). Tidligere utsagn nedenfor om at SC01 ikke er standard eller at PR172 venter, er historikk. Ingen ny spillutgave er publisert.

## Grenene, 27. oktober 2026

- PR #170 ble flettet av Tom 26.10 kl. 22:15, etter avsnittet under som sier at den ikke er flettet. Hele `screenshots.sh` var ikke kjørt (0 av 110) da; 27.10 kl. 19:15 består den 110 av 110 på main pluss SC01.
- `unreal-port-core` var avtalt integrasjonsgren for historiearbeidet (`HISTORIE_IMPLEMENTERINGSPLAN.md`), så PR #147 til #166 og #168 gikk dit med vilje. Main fikk det meste fordi grenen til PR #167 var laget fra `unreal-port-core` (til `41430ade`). #163, #164, #165, #166 og #168 kom etter det, og ingen flettet grenen videre, så SC01, planen og rapportene for SC01, T02 og NP1 ble igjen der. Loggpunktet kl. 14:53 og første utgave av regelen kalte det feil gren. Det var feil, og er rettet kl. 17:25.
- Toms beslutning: alle PR-er går mot main, og grunnlagsgrenen sjekkes før fletting (`AGENTS.md`). Flettingen er gjort på `ccr-30e38858-767d90` med PR mot main (konflikter løst, generatoren kjørt). 27.10 er editor og pakke bygget på Kubuntu, og SC01-kontrollene, HorrorStory-testene, hele `screenshots.sh` og gamle lagringer består (loggen kl. 19:20). SC01-R4 hadde ingen kode- eller assetrettelser i git utover `dbae3177`; bilens RT-LOD i gitignorert `Content/Free` må finnes lokalt ved pakking (lages av `Tools/freemodels/rt_lods.py`). SC01 er ikke installert som standard.

## Atmosfærepass ferdig, 2026-10-27 kl. 13:12 CET

- Gjeldende lokale standard er atmosfære-R1 med MetaHuman bevart. 1 435 sub-12cm steiner har ikke direkte skygge; større steiner/planter/bygg og modellavstander er beholdt. Grass_A/B bruker egen svak WPO, uten endring av den delte tre-/buskmatten eller eksisterende runtimeflora.
- Kveldens lydmodell hadde feil kulde før kl. 21 og et temperaturhopp. Kontinuerlig kveld og myk nattfase er rettet; eksisterende nattkurve, innendørsdemping og historieporter beholdes. Sju nye regresjoner består i samlet 1 593-selvtest.
- Fem målrettede Vulkan-prøver og sju kandidatbilder består. FSSS forblir av etter faktisk bilde-/GPU-sammenligning: omtrent 0,34 ms ekstra uten tydelig bildeutbytte. Småsteinssammenligningen ga bare 0,065 ms forskjell, ingen robust helrute-/FPS-gevinst hevdes. Se [rapport](ATMOSPHERE_2026-10-27.md).
- R1 er installert byteidentisk. Standardstarter på Low består ni kontroller med tre faktiske replikkklipp, pause, ansiktsrigg og diskrestore. Alle 70 personlige Saved-noder/50 filer er uendret. Canonical prune har pensjonert to eldre mellompakker; R1, to gamle fungerende R3-kopier og beskyttet SC01-R4 beholdes. Kilder/assets/Saved/bevis bevares.
- MetaHuman og punkt 5 er ferdig levert lokalt. Oppfølgingen er slettet i appen; samtalen er beholdt. Ikke gjenstart Claude, gjør dobbel implementering eller publiser ny spillutgave automatisk. Finere figur-/diner-/motelpolering og separat SC01 er egne senere arbeider.

## MetaHuman levert lokalt, 27.10.2026 kl. 12:32 CET

- Codex overtok etter Toms direkte bestilling. Gamle Claude-økter og ventere er avsluttet med kildene sikret. Gjeldende status er `metahuman_delivered` i `~/scratch-bm/Signal47-metahuman-20261026/takeover-codex/ownership.json`. Ingen annen motorjobb ble stoppet.
- R3 har fungerende MetaHuman med egen uniform, synlig rigid hårkortfrisyre, pust/blikk/blunk og tale på riktig aktive lydklipp. Kurver kopieres fra Body til Face/RigLogic. Alle 43 takes er assetvalidert; tre faktiske velkomstreplikker, pause og diskrestore med én figur er målt i pakket spill. Se [rapporten](METAHUMAN_WAITRESS.md).
- Samme R3 er installert i standardstarteren. 68 payloadnoder og alle 70 personlige Saved-noder/50 filer er kontrollert; installert Low-oppstart består åtte kontroller. Fem High-bilder er faktisk åpnet. Ingen menneskelig input-/lytte-/FPS-påstand, full merge-matrise, ny release eller GitHub-merge.
- Fem eldre genererte payloadsett er canonical-prunet. R3 og to eldre MetaHuman-pakker beholdes; SC01-R4 er særskilt beskyttet. Originaler, råassets, Saved, logger og bilder er bevart.
- Gammel `integrate.py --apply` er sperret for å hindre overskriving fra gammel staging. Videre klær-/portrett-/gestepolering er åpent. Avtalt automatisk neste arbeid er punkt 5 atmosfære og ytelsesrom, med SC01, historie, dag/natt og Saved bevart.

## Linux-nedlasting publisert, 26. oktober 2026 kl. 22:54 CET

- [Testutgave 26. oktober](https://github.com/Tombonator3000/SIGNAL-47/releases/tag/playtest-linux-2026-10-26) er publisert etter Toms bestilling for andre Linux-PC-er. To vanlige ZIP-er pakkes ut til samme Signal47-Linux-mappe; Play.sh starter spillet, Play-Low.sh gir Low/Clean/720p. Begge arkivene og tre metadatafiler er faktisk tilgjengelige uten innlogging.
- Kandidat og installert runtime er hashidentiske med terreng-R2. 43 leverte filer er dekomprimert og SHA256-kontrollert, og GitHubs fem assetdigester/størrelser stemmer. Arkiver: 1 468 082 463 og 2 003 099 541 byte. Ingen Saved, cache, debug eller råassets følger med. Ingen ny motorjobb eller mottakermaskinprøve. Bevis/leveringsverktøy: /home/tom/scratch-bm/Signal47-download-2026-10-26.
- PR170 er faktisk flettet 26. oktober kl. 22:15 CET, merge 0ee7c248033773907d4113c454432a7534819b4b. Den tidligere ventestatusen nedenfor er historikk. Fullmatrisen ble ikke kjørt på nytt; MetaHuman-prioriteringen er bevart. Nedlastingstaggen peker til denne mergen.

## GitHub-levering klargjort, 26. oktober 2026

- Tom ba om fletting til GitHub. Samlet kildeleveranse er pushet på `codex/saro-visual-route-20261026` i [PR170](https://github.com/Tombonator3000/SIGNAL-47/pull/170). Nyeste main med dokument-/miljøbilder er flettet inn uten endring av testet spillkode eller produksjonsassets. De seks private tekstene er integrert; JPG-/UV-binding er fortsatt separat arbeid.
- Publiseringsaudit består: 175 egne/CC0-uassets bruker Git LFS; private Fab-assets, personlige lagringer og 23 genererte kontrollfiler er utelatt. Begge narrative generatorer og HorrorStorys 20 tester består. HorrorStory-scenarioene er fortsatt forfatterkilder uten ny SC-runtime.
- PR-en er konfliktfri, men **ikke flettet til main**. Hele eksisterende `Scripts/screenshots.sh` kreves før merge. Den klargjorte kjøringen bruker samme R2-pakke, men er ikke startet. Tom svarte at MetaHuman-arbeidet må fortsette først; ingen automatisk teststart mellom Claudes jobber. Fortsett fullkontrollen etter faktisk frigitt Unreal-arbeid, og flett deretter samme verifiserte PR-head. Ny autorisasjon til vanlig merge trengs ikke.

## Terreng og natur langs ruten, R2 installert 26. oktober 2026

- Tom bestilte punkt 3 med flere agenter. Original Blender-geologi har nå 179 476 trekanter med sammenhengende erosjon, endret fjernprofil og glattere fjernnormaler. Samme veimasker, koordinater og spillbare høyder beholdes. De store sentrale/høyre trinnene i førbildet var far_northern_ridges; første R1 lot dem stå og ble avvist visuelt. R2 retter dette.
- Åtte eide Fab-modeller gir sju naturpartier, 84 solide steinfotavtrykk, 727 grus-, 184 gress- og 278 buskinstanser. Store steiner bruker målt plassering og eksisterende gang-/kjørekollisjon; smånatur har ingen egen skygge/RT. Grus ligger i varme uregelmessige flekker. En original 552-tri overgang dekker SAROs målte dekorskjøt. Ingen generell endring av gulv, historie, lys eller himmel.
- Native import har en lukket kjede på 40 private pakker. Originale Old West Learning-/JustBStudios-filer og staging er bevart; alle vendorbinærer er gitignored. Direkte PBR bruker faktisk inspeksjon av originalenes materialkanaler. Importen er idempotent og skiller uendrede MeshDescription-kilder fra automatisk regenerert render-LOD. Oppskrift/proveniens i Tools/routenature, original geologi i Tools/calichestreet/backdrop.
- R2 med binær `6d0ae58f452afcf45b459198588723676ad36bae8355787f4f83ac60537cf901` består seks faktisk åpnede sluttbilder, 1 581 eksisterende selvtester og én fysisk kontrollerstyrt Caliche/SARO-rute på 2 622,6 meter med inngang/telefon/diskrestore. 598 aktive RTCSV-prøver har maksimum 75,8328 MiB AlwaysResident under uendret 80 MiB-grense. Ingen generell FPS-, menneskelig input- eller fullnattpåstand.
- Samme R2 er INSTALLED_VERIFIED med 68 payloadnoder og alle 70 Saved-noder/50 regulære filer fra brukerens siste økt bevart. Standardstarter består tolv kontroller. Canonical prune har pensjonert tre gamle nærdetalj-payloadsett/21 filer; terreng-R2, eldre R1 og full nærdetalj-R4-rollback beholdes, med SC01-R4 særskilt beskyttet. Kilder/assets/Saved/bevis er bevart. Se [TERRAIN_ROUTE_2026-10-26.md](TERRAIN_ROUTE_2026-10-26.md). Bevisrot `/home/tom/scratch-bm/Signal47-terrain-20261026`. Ingen ny PR/merge/push; separat SC01 og MetaHuman er ikke del av denne leveransen.

## Detaljer tett på spilleren, R4 installert 26. oktober 2026

- Tom valgte punkt 2 etter Pure Sky. Tre originale Blender-modeller gir tolv SARO-armaturer med metallhus, fester, diffusorer og faktiske native lyskilder i riktige åpninger. Baldakinen har én kilde. Materialene har fem slots og modellene tre LOD-er. Kilder i `ArtSource/ExteriorFixtures`, import i `Tools/exteriorfixtures`.
- Native import består. Runtime erstatter sju gamle hus med 84 trekanter fjernet fra separate kopier av tre eksisterende Web-mesher; originalgeometri/materialer/kollisjon beholdes. Kun CPU-tilgang er aktivert i de tre native originalassetene. Cook inkluderer den nye mappen eksplisitt etter avvist R2. R3 har faktisk alle modellene i IoStore og readiness 7/84.
- R3 daginngang og nattlig fløyskilt er faktisk åpnet og godkjent. Monumentbildet utløste én lokal authored intensitetsendring 65→25 lm; gammel fallback beholder 65. Parkeringslyset har 350 lm med emitter utenfor veggen. Wing beholder 1 200 lm. Ingen global eksponering-/Lumenendring; alle seks skyassets er byteidentiske med Pure Sky R1.
- Etter brukerens spillavslutning er R4 bygget og pakket med binær `26673e1b033ca1defa3cc2a2b7a4736028fe20af3893616535fffad3729e2819`. Fem faktiske sluttbilder i tre prosesser er åpnet og godkjent: daginngang, nattparkering og faktisk strøm på/av/tilbake. Den nye Saved-baseline etter brukerens økt har 67 noder, inklusive 48 regulære filer; alle er bevart. Ingen brukerprosess er stoppet.
- `installation-r4.json` er INSTALLED_VERIFIED: alle 68 payloadnoder stemmer med prøvd kandidat. Faktisk standardstarter består elleve kontroller, og installert inngangsbilde er åpnet. Alle 67 personlige Saved-noder er bevart. Bevisrot `/home/tom/scratch-bm/Signal47-neardetail-20261026`; se [NEAR_DETAILS_2026-10-26.md](NEAR_DETAILS_2026-10-26.md).
- Verifisert full Pure Sky-backup ble brukt under installasjonen og senere pensjonert av canonical prune etter vellykket oppstart. Fire gamle payloadsett/28 genererte filer er pensjonert. Gjeldende R4, eldre R3/R2 og særskilt beskyttet SC01-R4 beholdes; kilder/assets/Saved/bevis er bevart. Ingen ny PR/merge/push eller endring av separat SC01.

## Pure Sky på dagtid, 26. oktober 2026

- Tom godkjente første visuelle steg med krav om uendret nattehimmel og stjerner. Kloppenheim 06 Pure Sky fra Poly Haven, CC0, er integrert som separat dagsmateriale og 8K HDR-tekstur. Det opprinnelige nattmaterialet brukes direkte ved null dagsvekt; fire originale sky-/stjerne-/horisont-/cloudassets er byteidentiske. Originale hendelses- og nattkurver beholdes.
- Full fotohimmel 07:00 til 18:00; myk utblanding 18:00 til 18:53. Eksisterende full natt ved 19:07 består. Innblanding 06:00 til 07:00 ligger etter eksisterende finale. Synlig himmel og skylight-capture bruker samme kilde; Low oppdaterer capture ved endret dagsvekt. Fotografiets skyer er statiske. Dette er ikke et nytt værsystem.
- Pakking og sammenligning mot R5 består. Fem kandidatbilder er faktisk vurdert: dag 17:20, mellomblanding 18:20, original himmel 18:53, natt 03:00 og Low dag. Naturligere dagskyer, beholdt nattutseende og stjernehull. Kildekurven er gjennomgått, men ingen hel sanntidskveld eller FPS-måling er gjort. [SKY.md](SKY.md) beskriver avgrensningen.
- R1 er lokalt installert med binær `548d4633…`. `installation-r1.json` er INSTALLED_VERIFIED, og faktisk standardstarter består åtte kontroller i `installed-smoke-r1/result.json`. Alle 68 payloadnoder stemmer med prøvd kandidat; 65 faktiske personlige Saved-noder, inklusive 46 regulære filer, er uendret. Verifisert R5-backup beholdes. Bevisrot: `/home/tom/scratch-bm/Signal47-puresky-20261026`. Tidligere R5-/NP1-funksjoner og separat SC01-arbeid bevares. Ingen ny PR, merge eller push.
- Canonical prune er anvendt: gamle ankomst/kontor R4/R3 har 14 genererte payloadfiler pensjonert. Pure Sky R1, R5-kandidat og R5-rollbackbackup er de tre beholdte komplette pakkene; aktiv SC01-R4 er eksplisitt beskyttet i tillegg. `delivery-r1.json` bekrefter installasjon og personlige lagringer også etter oppryddingen.

## Grafikkfunn lagret, 26. oktober 2026

- Tillegg i samme notat vurderer mest synlig løft per innsats: samlet himmel/lys, nærdetaljer og armaturer, terreng/natur langs ruten, én komplett UE Optimized MetaHuman, deretter atmosfærefinpuss. Første foreslåtte strekning er innkjøring til SARO-inngang. Poly Haven Pure Sky er en konkret kandidat, men nåværende Unreal bruker egen levende himmel uten SkyAtmosphere; Unity-arkivets Qwantani er ikke aktiv HDRI. Capture-grenen må vurderes sammen med synlig panorama. Dette er prioriteringsråd, ikke utført grafikkarbeid.
- [RENDERING_RESEARCH_2026-10-26.md](RENDERING_RESEARCH_2026-10-26.md) er oppslaget for Lumen Lite, FSSS og avstandsstyrte skygger, med kildehenvisninger og uprøvde forslag. «Probe exclusion» tolkes som Lite/probe occlusion. High/Ultra bruker vanlig sanntids-Lumen; Low setter GI=None selv om sg.GI=1 velger Lite-variabler. Ingen profil bruker aktiv Lite-GI. Epics omtrent 2x-påstand er ikke en målt dobling av spillets FPS.
- FSSS finnes innebygd som Experimental, men komponentflagget bEnableFSSS er av. Global r.Fog.ScreenSpaceScattering=1 alene aktiverer det ikke. Runtime-småflora er allerede shadowless; separat Caliche-sage, yucca og småstein er konkrete kandidater til kortere skyggeavstand. HISM-familier må ikke toggles etter avstand til én felles origo. Bevar store lysblokkere og skillet mellom skygger, modellvisning og GI/RT.
- Prioritet til senere: avgrensede småobjektskygger, eventuelt et eget Medium/Lite-nivå, deretter svak FSSS. Forslagene og terrengkoblingen er dokumentert, ikke implementert eller målt. Ingen nye UE-/Blender-/spillkjøringer, pakking eller innstillingsendringer i dette oppdraget.

## Parkering, inngang og kontor, R5, 26. oktober 2026

- R5 har to små besøksparkering-skilt, ankomst hvor som helst innen besøksflaten uten retningskrav, faktisk inngang via vestre rampe og kort ny gangsti. Normal stoppkontroll og faktisk lagret bilstilling består. Gamle stasjonære lagringer på området fullfører ordinær ankomst.
- Dales kontor kan nås før telefonsamtalen. Fysisk serviceskuff/nøkkel/dør, åtte originale Blender-assemblies, bedre flater/møbler og korrekt strøm-/lyskobling er integrert. Leste papirer overlever disk-/verdenslasting uten å gi uleste papirer eller fremtidige historieflagg. [Leveranserapport](ARRIVAL_OFFICE_2026-10-26.md) og `ArtSource/Office`/`ArtSource/ArrivalEntrance` beskriver kildene.
- Viktig baselinefeil: arbeidskopiens HEAD `41430ade` manglet allerede levert NP1 fra `2c05e33d`. R4 ble kortvarig installert, deretter tilbakeført til byteverifisert NP1 (`5621b29d…`) med alle 44 personlige Saved-filer uendret. NP1s eksakte runtime-/narrativdelta er nå flettet med de nye endringene. R5-binæren er `93e2fad3aa156325b8083159fc09828bc474731d9d05832263efb1c01cbfd890`. SC01-kandidaten er separat, aldri installert av dette arbeidet og skal beskyttes.
- R5: seks målrettede pakkede kjøringer PASS. Samlet oppstart/bilde-/RT-prøve inkluderer 1 581 selvtester og 21 bildeoppsett; tidlig kontor har 10 kontroller, Dale-dokumenter på Low har 191/5 diskrestores, LineLights 12/3 og himmel 16. Transvers gammel parkeringssave går ordinær inngangsrute og lastes på nytt. Ni utvalgte originalbilder er faktisk vurdert. Dette erstatter ikke menneskelig fullnatt, lyd eller fysisk Xbox.
- R5 RT: 596 aktive CSV-prøver, 77,2798 til 79,811 MiB AlwaysResident under uendret 80 MiB-varsel. Bare Office Room/Dressing bruker Nanite med full rasterfallback og separate 60 % RT-proxyer. Ingen global grafikk-/RT-grense er svekket. Etter Toms bekymring om testomfang ble siste runde begrenset til berørt NP1-funksjon, kombinert prøve og relevant parkering/kontor; ingen ny fullmatrise.
- R5 er installert: `installation-r5.json` er `INSTALLED_VERIFIED`, og `installed-smoke-r5/result.json` er PASS på alle åtte kontroller gjennom faktisk standardstarter. Alle 61 personlige Saved-noder, inkludert 43 regulære filer, er identiske før/etter. Canonical prune har pensjonert 70 genererte payloadfiler fra ti gamle pakker; R5, to eldre R4/R3 og aktiv beskyttet SC01-R4 beholdes. Kilder, Saved og bevis er bevart. Bevisrot: `/home/tom/scratch-bm/Signal47-arrival-office-20261026`.
- Fab 0.0.13 er installert under lokal UE 5.8.4/Engine/Plugins/Fab: 190 byteidentiske filer, Build ID 55116800. Native plugin og innlogging er faktisk brukt. Ørkenpakken `be5ed72e-f542-41f4-b747-bdb343eef609` er importert med pluginens + i separat `fab-plugin/FabStaging/Content/JustBStudios`: 518 pakkefiler, 1 977 336 454 bytes; native register viser 381 leverandørassets, inkludert 93 StaticMesh, 181 Texture2D og 3 World. Old West Learning viste «Install from launcher» i UE-pluginen; pakken er senere lastet ned med offisiell Linux Launcher Preview 0.103.0 etter at brukeren logget inn. Ingen ekstern downloader er brukt. Ingen Fab-plassering eller MetaHuman-figur inngår i R5. `fab-plugin/final-fab-report.json` dokumenterer fullført import, vedvarende filer og editorens avslutningsfeil etter import (returncode -11); editoren er stoppet, men ren avslutning er ikke bestått.
- Old West Learning er komplett i `/home/tom/.local/share/com.epicgames.eglv2/FabLibrary/Old West Learning-785da98f/OldWestLearning.uproject`: 3 227 filer/20 485 854 361 bytes, 3 207 uasset og 12 umap, ingen uferdige eglpart. Prosjektet er merket EngineAssociation 5.1 og er ikke åpnet eller konvertert i UE 5.8.4. `fab-plugin/old-west-launcher-report.json` dokumenterer fullført Launcher-nedlasting. Launcheren er åpen med brukerens innlogging; spillets kilde/innhold er uendret.
- `terrain-assessment.json` er en lesende vurdering uten implementering/test: behold sammenhengende grunn og bruk utvalgte stein-/klippemesher. Epics innebygde Mesh Terrain er Experimental og kan vurderes i ett avgrenset klippeparti. Ingen kjøp, pluginaktivering eller verdensmigrering er gjort; høydefunksjoner, historiegulv og delte SARO-/veikoordinater må bevares.
- Ved R5 gjenstod utvendige lampehus og montering, dokumentert i `exterior-light-audit.json`. Gap ved fløylamper, armatur/emitter inne i veggen og manglende baldakinmodell er senere rettet og kontrollert i nærdetalj-R4 over. VHS og samlet ny natt-/SC-koordinator står fortsatt åpne. De senere historiske S.1/S.3/S.6/S.7-notatene må leses sammen med NP1-statusen: forfatterkilder, frivillige tekster og dagens kapitteladapter er levert; ny SC-runtime er ikke levert her.

## Ku-modeller levert lokalt, 26. oktober 2026

- Konseptbilder og originale stiliserte Blender-modeller finnes i `ArtSource/Cows`. Tre native assets under `/Game/Signal47/Art/Cows`, hver med tre LOD-er, erstatter gammel levende/død kugeometri uten ny lagringslogikk.
- Samme R2 er installert i standardspillet, binær `60e63f4e...`. 1 437 selvtester, ti pakkede scenarioer, ekstra Low-nærbilde og installert oppstart består. Alle 40 personlige Saved-filer er bevart. Gjeldende pluss to eldre testpakker er beholdt.
- Pakket RT er målt til 79,3446 MiB i et avgrenset aktivt sample, med hardware RT og uendrede terskler. Editorens 82,922 MiB inkluderer ekstra editorgeometri.
- [COW_MODELS.md](COW_MODELS.md) har filer, bevis og begrensninger. Mørke oversiktsbilder beviser ikke full visuell ni-til-åtte/026; lesbart øremerke/kjeve er ikke pixelattestert fra femmeterskameraene. Lokale kildeendringer er ikke pushet/flettet.

## Miljøgrafikk levert, 26. oktober 2026

- `ArtSource/EnvironmentGraphics` inneholder 48 originale JPG-flater: seks kinoplakater til Plazas to utvendige og fire innvendige rammer, avskjedsark og billett, 12 lokale annonser/menyer, ti emballasje-/småtrykkflater, seks hverdagspapirer, ni arbeidsplasskilt/-blanketter og tre nabolagslapper. De supplerer de 31 tidligere dokumentbildene, totalt 79 rekvisittmotiver.
- Finalene bruker 23 425 648 byte mot PNG-originalenes 149 498 003 byte, 84,3 prosent mindre. Progressive sRGB-JPG med 4:4:4, kvalitet 82, 84 eller 86, maksimal langkant 2048 piksler og hver fil under 650 KiB. Originaler og absolutte genereringsstier beholdes utenfor Git. Katalog, fullprompter, oversikt, offlinegalleri og kontrollfiler følger pakken.
- Teknisk kontroll av alle 48 JPG-er og uavhengig visuell nærlesing av 96 katalogpunkter består. Galleriets 48 bildefelt/lenker, sju gruppefiltre og åpning av trykktekst er prøvd i lokal nettleser; ingen ødelagte lastede bilder. Kontrollen bekrefter kildeassets, ikke senere bruk i Unreal.
- Stedsnavn, priser, åpningstekster, 1420 AM og SAROs fullnavn følger eksisterende kilder. Kinoplakatene fastsetter ikke nedleggelsesår. Fiktive produktmerker oppretter ingen nye butikker. Blanketter er tomme, kvitteringer er bakgrunnstrykk, og veikartet er bare et omslag. Foreslåtte plasseringer og mål er importveiledning.
- Leveransen er flate fargeteksturer til senere import. Nye 3D-modeller, UV-/materialbinding, fysisk plassering og runtimeprøve inngår ikke. Dokumentenes eksisterende lesegater og hele spillinstallasjonen er urørt.
- Felles Documents-konvertering bruker nå en stabil sRGB-profil slik at identiske kilder gir identiske JPG-bytes mellom prosesser. Alle 31 tidligere JPG-er, katalog-/genereringsdata og historiske kontroller er bytebevart. Galleribyggeren støtter miljøpakkens egne gruppenavn og tekster med eksisterende standarder bevart.

## Dokumentbilder levert, 26. oktober 2026

- To nye brukerleverte obduksjonsmotiver er logget separat i [Documents/References/DaleAutopsy](ArtSource/Documents/References/DaleAutopsy/README.md) til mulig senere bruk i Dales private filer. JPG-kopier, motivbeskrivelser og kildehasher er bevart. Opphav/dato/sted og autentisitet er uavklart; ingen hendelses- eller runtimekobling er opprettet. De to referansene inngår ikke i de 79 ferdige rekvisittmotivene.
- `ArtSource/Documents` har 31 originale imagegen-rekvisitter som progressive sRGB-JPG: mappeomslag, alle sju EveningShift-ark og seks private Dale-utkast, 13 senere arkivark og fire nye valgfrie bakgrunnsark. Galleri, transkripsjoner, prompter, kildehasher og kontrollverktøy følger med.
- JPG-filene bruker 12 143 169 byte mot PNG-originalenes 89 438 281 byte, 86,4 prosent mindre. Originalene er bevart utenfor Git. Konvertering endrer ikke tekst eller beskjæring.
- 31 tekniske kontroller og uavhengig visuell nærlesing av alle finaler består. E15 følger crop B-komposisjonen som generert nytolkning; det er ikke en pikselidentisk kildebeskjæring. E09A er kvalitativ figur, ikke et nytt målsatt overlegg. E10 dateres i tittel/katalog; kildebrødteksten har ikke trykt årstall.
- Dette er bildeassetproduksjon; den utførte ingen bygg, pakking eller installasjon. Den separate NP1-/R5-leveransen har allerede registrert de seks private dokumentene i EveningShift, med tekstvisning, skalering og faktisk lagret lesestatus. Det som gjenstår for JPG-pakken er import, UV-/materialbinding og eventuell bildevisning eller dokumentert bruk som fotobevis. VHS72sek er fortsatt en separat, uprodusert filmoppgave.
## SC01 pakket teknisk aksept, 26.10.2026 kl. 13:18 CET

Kilde `dbae3177` på `codex/story-sc01` har 21 isolerte pakkekjøringer og 32 faktisk vurderte originalbilder. High/Low 120/17 hver, oversett hånd 112/16, begge komplette ankomstruter, fem ferske vanlige Continue og fokuserte T01/T02/kontor/LineLights/felt/lab/finale-regresjoner består. Pakket selvtest 1 501. RT 79,8043 MiB i 197 oppvarmede samples, uendret 80 MiB-grense. Full hånd-/bil-LOD0 og originalressurser er bevart, med ordinære reduserte LOD1. Fem takes fullfører faktisk telefonruting; SARO/Sorrow-ASR og menneskelig lytting står separat. [SC01_RUNTIME.md](SC01_RUNTIME.md) fører kilde-/binær-/bevisbinding og bildegrenser. PR, fletting og installasjon gjenstår; NP1 er fortsatt standard. H01 forblir åpent til SC02. Senere scenarioer er ikke produsert.

## SC01 overtatt og pakket kandidat under bygg, 26.10.2026 kl. 11:41 CET

Aktiv gren er `codex/story-sc01` i `/home/tom/scratch-bm/wt/horror-h01`. Kodecommit `f935b237180af1cd5c1aca31cf683dcbc4338b82` har SC01-koordinator, alle manus-ID-er registrert, faktiske modeller og fem stemmetakes. Etter rettet filmreserve består editorprøven 120 kontroller, 17 disk-/Flow-restores og 139,99 m gange. Pakket kandidat `Signal47-sc01-20261026/candidate-r1/Linux` bygges fra 14 006 frosne kilde-/Content-filer, med før-/etterkontroll og separat byggekvittering. Ingen pakket eller installert SC01-aksept er ført. NP1 er fortsatt installert standard. Den gamle chatten stoppet med modellkapasitetsfeil; Tom ba denne chatten overta hele det autoriserte arbeidet.

## Godkjent historieplan og arbeidsstart, 26.10.2026 kl. 09:36 CET

[HISTORIE_IMPLEMENTERINGSPLAN.md](HISTORIE_IMPLEMENTERINGSPLAN.md) er styrende gjennomføringsplan for alle13 hovedscenarioer og79 eksisterende enkeltscener. Tom valgte full produksjon, separat commit/PR/fletting og installasjon per SC, samt spillbar eksisterende fortsettelse mens senere scenarioer mangler. Hver lagret sak beholder sin innholdsrevisjon uten nye bakoverrettede krav. SC01 er under arbeid i isolert arbeidskopi `/home/tom/scratch-bm/wt/horror-h01`, gren `codex/horror-h01`, fra core `1c929b76`. Alle senere SC og alle samlede H-kort står åpne. H01 krever SC01/02, H02 SC02 til05, H04 SC07/08 og H05 SC08/09; øvrige lukkepunkter står i planen. Ingen planstatus er spillaksept; NP1 er fortsatt dokumentert installert standard.


## NP1 er gjeldende installert standard, 26.10.2026 kl. 01:26 CET

PR164 er flettet i93edb7a4; testet kilde2c05e33d og samme pakke er installert.17 pakkede kjøringer,1 493 selvtester og28 vurderte originalbilder. Standardstarterens smoke,40 identiske payloadfiler,40 byteidentiske personlige Saved-filer/57 noder og full gammel ku-R2-backup består. Sluttpayload/Saved er kontrollert igjen etter smoke/pruning. Gjeldende NP1 pluss backup og original ku-R2 er tre komplette testpakker; gamle ku-R1/T02-payloads pensjonert med kilder/Engine/Saved/bevis bevart. S.1/R2.1 er produsert manusgrunnlag, S.6/S.3a/S.7a er avgrenset runtime. Ny kveldskoordinator, møter, VHS og menneskelig fullnatt står åpne. [NIGHT_PRODUCTION_RUNTIME.md](NIGHT_PRODUCTION_RUNTIME.md) og bevisrotens NP1_DELIVERY_COMPLETE.json er nyeste sannhet.

## NP1 pakket aksept, 26.10.2026 kl. 01:12 CET

Kilde `2c05e33d18156c85df174c5afcc0f4030de38878` har17 pakkede kjøringer,1 493 selvtester og28 faktisk vurderte originalbilder. Dales High/Low har191/5 hver; LineLights12/3, Sky16+full Flow per High/Low, T0153/19, T0265/15 og38,89m gange. Feltfilm/mørkerom og hele finalen består. RT79,3446MiB i625 aktive samples ligger under uendret80MiB-varselgrense. Akseptfilen binder21 rapporter. S.1/R2.1 er forfattergrunnlag, S.6 og runtime-delene S.3a/S.7a er levert kandidat. Nye SC-/kveldsporter, fem aktive møter og original VHS er fortsatt åpne. Ku-R2 er bytebevart. Installering av NP1 skal fortsatt verifiseres; ikke anta at en pakket kandidat allerede er standard.

## Kontorfunn og himmelkandidat, 26.10.2026 kl. 00:42 CET

Nyeste arbeid foregår isolert på `codex/night-production`. HorrorStory har nå10 hovedoppdrag,13 scenarioer,79 scener og16 dokumenter med20 kildekontraktprøver. EveningShift har101 scener/17 dokumenter og uendrede109 takes. Seks frivillige loretekster og et separat arrayserviceark er faktisk lesbare i kontoret; lesestatus og legacy/T02/VHS-grenser bevares. Dokumentets store tekst er1,35 med fysisk rulling. LineLights skiller opptrådt, faktisk sett og avsluttet, og gjenopptar fra disk. Fem optional himmeltrinn bruker fysiske baner, siktsjekk og separat versjonert lagring. De knyttes foreløpig til faktisk kapittelprogresjon, ikke en ferdig SC01 tilSC13-koordinator.

Gjeldende installasjon ved oppstart av leveringskontrollen var allerede ku-R2, binær `60e63f4e7cac1afd5097534cdf885d009f73705dbe892d41c63fc991949da4c9`. Dens140 kilde-/assetfiler er tatt inn uendret fra verifisert snapshot, uten å redigere den andre chatens arbeidsmappe. Ny kandidat må testes og inkludere disse før standard erstattes. [NIGHT_PRODUCTION_RUNTIME.md](NIGHT_PRODUCTION_RUNTIME.md) fører gjeldende test-/leveringsstatus.

## T02 levert og installert, 25. oktober 2026

Kilde `5b9fcb3c` er flettet i PR162 (`d152fb6d`) og samme Linux-pakke er installert via standardstarteren. Den fysiske serviceskuffen, egen Operations-nøkkel, dør/terskel, D02-papirer og feltmappe leder til T02s faktiske normalreferanse, ståsted, fire kontroller, eget originalfoto og én SARO-arkivering. Uavklart utfall beholder hovedruten. Kontoret krever ingen sett VHS; STATION 01, Nora, K3, C og byforsyning beholder sine roller.

15 pakkede kjøringer består: 1 437 selvtester, T02 High/Low 65 API/15 diskrestores hver, uavklart 30/6, eksisterende T01 53/19, sammenhengende native tur 41 kontroller/fire lagerlesere uten mellomlast, fokuserte kapittelprøver og fem vanlige ferske Continue. Native målte 932,2 m ut, 966,7 m retur og 124,1 m gange. De fire native lagerleserne er Writer/fresh-reader, uten Flow-omlasting. 23 akseptrapporter og 25 faktisk sette originalbilder er hashbundet. Endelig binær er `7531698e9695c595ddba2868ab37e2f839c16a82d7e47b77e42f8c2cfdbaa86b`. Ingen CI-jobber var rapportert for eksakt PR-head; dette er lokal aksept.

Arkivkortets originale tittel, stempel og side gjenopprettes fra faktisk historikk. Manglende fotofil gir ikke bekreftet credit, erstatter ikke originalbildet og tillater ikke ny arkivering av samme forsøk. 40 installerte payloadfiler og alle 40 personlige Saved-filer (57 noder) er etterkontrollert uendret også etter installert smoke og pruning. Gjeldende T02, full T01 R2-backup og T01 R2-kandidat er de tre komplette testpakkene som beholdes. To eldre payloads er pensjonert, 7 621 635 760 logiske bytes; kilder/Saved/Engine/logger/bilder/bevis er bevart.

[T02_RUNTIME.md](T02_RUNTIME.md) og `/home/tom/scratch-bm/Signal47-t02-20261025` har bevis og grenser. Fersk Continue beviser normal last, bevarte kildestorer og scene/HUD, ikke en fersk minnedump eller sletting av tidligere positivt flagg. Foto identifiserer ståsted/utsnitt; mørke ledere og lampestatus avledes ikke fra print alene. Previewbildene er delvise sider. Original 72-sekunders VHS, nye forfatterfunn, samordnet helrute, skyggemann/stealth og menneskelig Roam/Xbox/lytting/fullnatt står fortsatt åpne.

## Sammenflettet hovedscenario, 25. oktober 2026

- Nyeste bestilling: flett alle designrundene sammen, behold eksisterende randomhendelser, vis ett tidlig UFO-lys og flere etter hvert, og gjenbruk tidligere Dale-kontorideer med lesbare UFO-/aliendokumenter.
- [MISSION_FLOW_1800_ROSWELL.md](MISSION_FLOW_1800_ROSWELL.md) er nå én hovedrekkefølge SC01 tilSC13, ikke nye overlappende revisjoner. Tidligere82 406bytes er bevart med SHA-256 i [MISSION_DETAILS_REFERENCE.md](MISSION_DETAILS_REFERENCE.md). Alle32 NATT-kort/seks valgfrie spor finnes der. Eldre memory-/loggoppføringers «del0/del0B» gjelder den bevarte arbeidsversjonen.
- Fem skrevne aktive møter og tre ulike Calichebesøk består. SC04 får faktisk lokal plansperre, prøvd retrett fra bil til SAROs kontrollrom og en avgrenset loggsikring med Ward på linjen før residualen. Troverdig motivasjon og faktisk transport er fortsatt uprøvd.
- SKY01 til05: ett kryssende lys under første utvendige runde, to fra Caliche, flere i ulike sektorer, ordnet feltbevegelse, spredte grupper før morgen. Ingen kameratvang/kvote/gratis observasjon. Gamle LineLights, StarHole, komet og meteorer beholdes. Lagre global progresjon og faktisk observert separat.
- Randominventar er kildekontrollert og beholdes, inklusive gammel tapt-tid-bortføring. Gammel pool kjører ch1 til5 og bybaseline sperrer gamle kveldsvarianter; ny akt trenger eksplisitt fasekart. Utvid eksisterende opptattvern og versjonert Fixed→NextFixed-budsjett. Behold gamle saves og korrekt avbrudd/gjenopptakelse. Ny fangst er lokal retry uten gammel teller/etterspor. LineLights' avbrudds-/observasjonsfeil har egen åpen oppgave.
- [DALE_OFFICE_DOCUMENTS.md](DALE_OFFICE_DOCUMENTS.md): sju eksisterende dokumenter, seks nye frivillige engelske tekstutkast og tydelig opphav/lesestatus. Feltmappe/indeks bærer hovedspor; EV_DALE_CONTACT er kontaktinstruks, ikke fotografisk kontaktark. Arrayet trenger nytt servicevedlegg; eksisterende mappeindeks beholder T02s ene kulvert-/kameraståsted. VHS72sek er manus, ikke produsert/spillbar film.
- Kanonpresiseringer: Ward er kvinne. Nøkkelen i kruset er opplysning i Tomás' historiske brev, ikke en nåværende nøkkel/adgangsgate. Operations-reserven er en separat faktisk rekvisitt. Ankomst13. april18:00, residual00:13:47,1947-tider, fjernark04:04 og én finale05:29 beholdes. Residualen må kunne starte passivt uten å oppheve SC04s automatiske plansperre.
- Ved designleveransen 25. oktober var S.1 tilS.9 åpne og HorrorStory hadde ti hovedkort/36 scener. Senere forfatterarbeid har levert 13 scenarioer/79 scener og sju kontrollerte lesefiler, med 20 beståtte kildekontraktprøver. NP1 leverte blant annet kontortekster og dagens himmeladapter; den nye sammenhengende SC-runtime, fysiske møter, opptak og menneskelig fullnattprøve gjenstår. Se gjeldende leveransestatus øverst og Narrative/HorrorStory/README.md.

## Integrasjonsforslag for array og Caliche,25. oktober2026

- Tom ber om kobling til eksisterende historie, ekstra oppdrag på selve arrayet og faktisk byfunksjon. MISSION_FLOW del0B samler forslaget etter brainstorming, lokal game-production og en Gauntlet-kritikkrunde. Dette er en anbefaling, ikke ferdig runtime eller påstått brukervalidert design.
- ARRAY-01 etter arkiv/BY02 før STATION01: fast kilde blir stående, operatøren velger A→B/B→A og kort retur, nærmeste antenner følger faktisk posisjon, holder tom plass kort og returnerer026. Fysisk observasjon/landemerker støtter geometri; opptaker støtter bare lyd/tid; kommando-/drivlogg er egen kilde. Dette peker mot videreført1947-oppmåling uten å løse C eller Noras motiv.
- Tre Calichebesøk gjør ulikt arbeid: BY01 inntrengermelding/utkalling og avtalt hjelper/bortføring; BY02 datert Earl/1981-kildekontroll/feltopplegg; BY03 trygg sammenstilling, readback, E15/P13 og risikovurdert avreise. Førmidnatt-skyggemann flyttes til kjent byvindu/verksted ved samme besøk for å unngå flere pliktturer.
- Aktivt trusselmøte4 legges til arrayet. STATION01 beholder lokal nullprøve, kabel, Tomás og kort avskjæring. Behold fem aktive møter totalt og den faste finalen. Byen er reell tilflukt; retur til SARO har et avgrenset sikkerhets-/kontrollformål og må prøves som troverdig handling.
- I.1-I.6 er åpne for senere scener/kilder, fysisk arraytilgang, oppdragsfase uten gammel Force-bortføring, byhandlinger og prøve. Ingen egne runtime-/T02-/pakke-/installasjonsendringer i dette designarbeidet.

## Skrekkrevisjon R2, 25. oktober 2026

- Tom mener første oppdragsplan er anemisk, med én alienhendelse og nesten tom kveld18-24. Revidert hovedretning i MISSION_FLOW del0 og HORROR_STORY_PLAN: fem nødvendige spillbare trusselmøter, tre før midnatt, pluss synlig vitnebortføring og én fast finale.
- Kveldsbeats: nærvær utenfor glass18:15, faktisk trafoflukt19:00, svart figur inne20:15, tilkalt kveldshjelper tatt21:00, annex-inntrenging22:15 og aktivt bilmøte23:20. Deretter feltmøte ved STATION01 og størst samordnet SARO-beleiring etter04:04. Tidene er forslag og krever ingen passiv venting.
- Reyes melder fare tidlig og søker hjelp/avreise. Jobben blir sikkerhet/kommunikasjon/dokumentasjon. Lokal06:00-serie på hold tidlig; H08 kvitterer fortsatt suspensjon og tiltak ved andre stasjoner. B-12/nullprøver tester mekanismen, ikke om alt bare var en lampe.
- Bevar00:13:47 som første store instrumentelle residual, avtrykksregel, svart skikkelses usett-flytting og05:29-finalen. Reaktive fremmede kan sees tidligere; slutten avslører hva registreringen gjør med Reyes. R2.1-R2.9 er åpne for senere produksjon. Den36sceners engelske pakken har ennå ikke de nye kveldsscenene. Ingen ny runtime/lyd/modell/bygg/installasjon i denne revisjonen.

## Detaljert oppdragsreise 18:00 til Roswell, 25. oktober 2026

- Nytt detaljvedlegg: [MISSION_FLOW_1800_ROSWELL.md](MISSION_FLOW_1800_ROSWELL.md), 32 arbeidskort under H01 til H10 og seks valgfrie spor. Hvert kort har løsning, fare/ro, utvei/recovery og forbindelse videre. Ingen survival-ressursøkonomi, tilfeldig anomalikvote eller nye slutter.
- Bygger på levert T01 R2 og nyere engelske HorrorStory-kilder. Forfatterpresiseringer: ufullført T01 tillater kalibrering, men sperrer Survey til faktisk kontroll/filing i nye saker; E15 er gammelt, innrammet 1947-utklipp uten 04:45-port. JSON-kilder rettet og generert lesestoff oppdatert.
- Aktive kveldsoppgaver må skje før siste notch/Survey som starter dagens automatiske K3/dip/tidshopp. Fjernarkenes fulle intervall er til04:04; første beleiringsversjon sikrer mottakeroriginal, påbegynt hendelsesark og ferdige fjernark etter dette og før angrep. Romobservasjoner føres først etter hendelsen. Ekstern sikring er konkret diktering/readback av kilde-ID/måletabeller til Wards daterte logg; foto/negative blir i mappen.
- To B-12-kontroller er gyldige, P07 er normal lokal nullprøve, og avtrykk svarer ikke på nye spørsmål. C8.15 og ment overlevering10.4 beholdes forskjellige. Varsling lykkes før den ene faste finalen05:29:00 til05:29:47.
- H1.5 til H1.7 og H6.5 står åpne i todo for delscener, rekvisitter, faktisk tids-/transportinnpassing og menneskelig prøving. Bare dokument-/forfatterarbeid nå. Pågående T02-spillkode er ikke endret av denne leveransen.

## T01 R2 levert, 25. oktober 2026

- `codex/t01-service-route`, kilde `85aee475`, fra core `f60d30eb`. R2-pakken har binær `81af1808a19e89b805de8093a4863d9005345d24c613cb96039024e8dd710de9`, 40 payloadfiler og 1 839 frosne kildefiler. Alle 14 pakkede funksjonskjøringer og 11 faktisk gjennomgåtte originalbilder består på samme pakke. PR160 er flettet fra head `656b283d` i `d4a25411`, og samme R2 er installert. Alle 40 payloadfiler og 40 personlige Saved er kontrollert; installert smoke og etterkontroll etter pruning består. Ingen CI-jobber var rapportert for eksakt head.
- Ny kveldssak gjør journal, faktisk kontroll og lampesammenligning før én fysisk SARO-filing. RX0 til1 er fri, RX1 til survey venter. Gamle/ukjente/no-version-saker får ingen ny gate. Credit valideres etter den aktuelle controller/routine. Etter filing/Continue vises faktisk neste RX-handling og notch-målet tømmes ved Survey.
- Pakket selvtest 1 424, kontrakt 53 API/19 store-leser/full-Flow-restores, native 10 kontroller/932,3 m ut/966,7 m retur/55,4 m gange, sju rene nye Continue-prosesser, mat/D01 og tre målrettede billedscenarioer består. Faktiske observed-/filed-storer har samme fysiske resultat/tid og false til true arkivering. R1s tre tomme HUDmål er reprodusert og rettet i R2; R1-rapporten forblir FAIL. Første native-editorganglinje gjennom trucken og UTF8-only scratch-parsere er bevart som feilbevis, ikke omdøpt.
- Trygg pruning er gjennomgått og anvendt i idle: R2, R1 og full hashverifisert Q3-backup beholdes som tre komplette testpakker. Tre gamle kandidatpayloads er pensjonert, 10,647 GiB logisk størrelse; kilder/Saved/Engine/logger/bilder/bevis er bevart. Dette garanterer ikke fysisk frigjorte bytes.
- [T01_RUNTIME.md](T01_RUNTIME.md) har omfang/grenser. Bevisrot `/home/tom/scratch-bm/Signal47-qa-fixes-20261025`. Native har D01/mål/start-fixtures og ingen flytting/lasting etter START; manuell Roam, mus/Xbox, lytting og ubrutt fullnatt er ikke prøvd. Ingen ny hel110 eller VSM-ytelsespåstand.
- H2.1a er runtimeintegrert/testet, men H2.1 samlet, T02, kontor/VHS, skyggemann og stealth er åpne. Neste konkrete kildekart ligger i `t02-office-preparation-v1/PLAN.md` med 36 hasher: ekte kontor/dør/terskel, egen Operations-nøkkel/feltmappe og fire faktiske T02-produsenter mangler. Ny original72s VHS og egen pause/strøm/save-transport trengs, uten å blokkere hovedruten. STATION01/Nora/K3/C bevares.

## QA-retting Q3 levert, 25. oktober 2026

- Arbeidsgren `codex/qa-fixes-horror-production`, testet kilde `d87af618`, PR158 flettet i `d31dc112`. Bevis i `/home/tom/scratch-bm/Signal47-qa-fixes-20261025`. [QA_FIXES_2026-10-25.md](QA_FIXES_2026-10-25.md) har gjeldende status. Samme Q3 er installert med 40 payloadhash og 40 byteidentiske personlige Saved-filer. Installert smoke med isolert profil består; Saved er fortsatt identisk etterpå.
- Kvalitetsbevarende Vulkan-omgåelse: `r.Vulkan.Bindless.PreferredExtension=1` før RHI-init velger støttet descriptor heap på RTX3080/595.99.02. Samme installerte Low-baseline feiler uten og består med dette ene flagget. Q1 består komplette High/Clean- og Low/Clean-finaler helt til meny. Ingen himmelshader eller motorfiler er endret; unsupported hardware beholder motorens descriptor-buffer-fallback. Universell driverrotårsak er ikke bevist.
- Åtte eksklusive eksporterte LineSegments-ribboner bygger ikke RT-strukturer. Q1 måler 78,1538 MiB AlwaysResident i 349 aktive kontrollromsprøver, under uendret 80 MiB-grense. Runtime materialaudit har null fallback/mappingfeil; egne antenners 33 seksjoner er kontrollert. VSM-oppstartskøen er fortsatt profilgjeld, ikke løst av disse endringene.
- FRAME03/04 er tydelige i faktiske lagrede High/Low-JPEG-er etter synkron native blits. Dump/store-bytes og targets består kontroll. QA03 til QA09 har konkrete kildeendringer. Q2 retter påvist fokusfeil i utgangsvarselet; faktisk Esc/default Enter/Xbox B/Down+A/Down+Enter, skrivefeil, settingsreopen, metadata/controls, ombundne navn og P13 består. Q3 retter automatisk kortoverdekking ved stor tekst uten å flytte brukerlagrede kort. Fem målrettede brettbilder/Slate-pan/Esc og fem nye API-geometrikontroller består.
- Q3: 1 424 selvtester, hele kanoniske 110/110 på samme binær og komplette High/Low-finaler til meny består. To ekstra High/Carpenter-bortføringer har ulik RNG og ingen GPU-feil. Native gammel cache er faktisk lastet og de sju originale installerte cachefilene er uendrede. Matrisens 197 aktive RT-prøver er 78,1538 MiB AlwaysResident med uendrede 400/20/true. Ingen ny device-lost i dette omfanget, ingen påstått generell krasjprosent eller FPS-garanti.
- H1.2 til H1.4 er engelske forfatterkilder i `Narrative/HorrorStory`: ti oppdrag, 36 scener og fem dokumenter; generator og eksisterende EveningShift består. Ingen nye opptak, ny runtimehistorie eller stealth er produsert. H1.1 menneskelig måling, H2 til H6, fysisk Xbox, lyd og ubrutt ny sak står åpne.
- Neste T01-reise er konkretisert som ukompilert, uprøvd scratch-utkast utenfor Q3-kilden. Det kobler eksisterende fysiske journal/kontroll/lampe til nye sakers Shift og én returarkivering før survey; gamle saker får ingen ny gate. T02 trenger kontorreferanse og reelle kontroller og er ikke levert.
- Prune-plan er kontrollert og anvendt i streng idle: Q3 og Q2/Q1 er de tre komplette testpakkene. Fire gamle payloadkopier er pensjonert, 14,187 GiB logisk størrelse. Backup-payloaden fra PR157 er blant dem; manifest/Engine-filer og alle Saved/kilder/bevis er bevart, men den gamle backupen er ikke en komplett kjørbar pakke.

## Spilltest av installert PR157, 25. oktober 2026 kl. 15:03 CET

- Tom ba om full spilltest, feilrapport og forbedringer. [SPILLTEST_2026-10-25.md](SPILLTEST_2026-10-25.md) er ferdig: installert kilde `1e89ae00d07b1e891489cd44eeae8412094d9322`, 110 matrisescenarioer med 108 PASS, 13 tillegg med 12 PASS og separat Low-finale FAIL. 1 405 interne selvtester PASS. Sekvensielle guardede jobber, egne profiler, ingen tung konkurrerende jobb.
- Fire unike Vulkan device-lost-krasj: én bortføring og tre finaler. Gammel High/Carpenter, ny High/Clean og ny Low/Clean feiler 7,1 til 7,6 sekunder inn i uttrekket etter 05:29:47. Ingen full finale nådde sluttittel/credits/foto/meny. Low er ingen omvei; rotårsak er uavklart. Den tidligere avgrensede skycapture-rettelsen beviser ikke at hele spillet er stabilt.
- Direkte åpning uten mat, burger/sandwich/vann, parkering/overlevering, forberedt kapittel 1 til 2, delvis injisert kapittel 3 til 6, frisk Continue til kapittel 5 og nyere anomaly-kontroller består. Fotoassets er bevart ved Continue. Normal kapittel 2 til 3, fri fullnatt, lyd og fysisk Xbox er fortsatt UNVERIFIED. Anomaly-vinduets svake andre form er ikke sikkert visuelt identifisert; pappfigurens ekstra form og ON AIR kan sees.
- Prioriter GPU-krasj før større utvidelser. Rapporten har 11 QA-funn og åtte forbedringspunkter: motorvarsel, aktive ISM-materialvarsler, mørke feltfoto, lagringsvarsel, bindingsnavn, tilgjengelighet, kartbevis, tekstfeil, reisemotivasjon, repetisjon og personmodeller. Ingen ytelsestall eller målt menneskelig spilletid hevdes.
- Ingen spillendring/ny pakke/installasjon. 40 installerte payloadfiler, 40 personlige Saved-filer og 411 kilde/config/scriptfiler er før/etter-identiske; HEAD uendret. 58 unike fullbilder/foto og sju kontaktark faktisk vurdert. Bevis: `/home/tom/scratch-bm/Signal47-playtest-20261025`.

## Samlet historie- og skrekkplan, 25. oktober 2026

- Tom ønsker cosmic investigation / psychological hide-and-seek / stealth horror, gradvis kommende invasjon, senere jumpscares og en solid loop. Han ba deretter om å legge planen i todo for senere innføring. Detaljplanen er [HORROR_STORY_PLAN.md](HORROR_STORY_PLAN.md); H0 er dokumentarbeid, H1 til H6 er åpne produksjonsetapper øverst i todo.md.
- Anbefalt signatur: kjent SARO bades i eksternt hvitt søkelys, fronten blir ubrukelig, én reaktiv inntrenger kommer inn og spilleren bruker undersøkte lys-/skjulregler og to ruter til servicebilen. Et funn skal endre en fysisk handling. Ingen survival-ressursøkonomi.
- Skyggemannen beholder stabil usett-flytting etter ANOMALY_PLAN del 13. Inntrengeren er en annen visuell/atferdsmessig kategori og kan bevege seg under blikk. Nåværende kode har ikke et ferdig stealth-system; Walker/dekning, sansing, søk og lagring er eget kommende arbeid.
- Bevar Nora/Tomás/1947, avtrykk som ikke svarer, A/C-skillet, 1420.405 MHz, 27 antenner, az 026 og én finale 05:29:00 til 05:29:47. Invasjonen er et bevisst nytt fortolkningslag med nødvendige hovedrutespor. Varsling og siste reisemål må få en troverdig begrunnelse.
- Kun plan og oppgaver er innarbeidet. Ingen ny runtime, modell, lyd, bygg, pakke eller installasjon. Frykt, stealth, ny total spilletid og den nye beviskjedens spillerforståelse er UNVERIFIED.

## Skyggemenn, 25. oktober 2026

- Tom ønsker helt svarte menneskeskikkelser som bare flytter seg usett, og ba om flere agenter til psykologi-/skrekkvurdering. Tre parallelle lesegjennomganger og primærforskning er samlet i ANOMALY_PLAN.md del 13; produksjonssteg ligger i todo.md.
- Forslag: stabil observasjonsregel, konservativ frysing også i periferien, ingen forsvinning ved zoom/lykt, beskyttede paneler/dialog og en hel SARO-vindu/trafo/retur-pilot med foto/lagring. Gamle Glimpses og SaroDark har motstridende forsvinningsregler; de er fortsatt uendret.
- Kun design og dokumenter. Ingen skyggemannmodell, ny runtime, bygg, pakking eller installasjon. Frykteffekt, spilletid og ytelse er ikke prøvd. Eksisterende M6 beholdes.

## SARO og resterende modellprioritet, 2026-10-25 kl. 02:07

- Testet spillkilde `aefd18157be7502ac98d3e1695b4c4fcdd7d26a0`, kandidat `M6`. SARO har 2 hovedmaster og 1 service-/trafostolpe innen 100 m; bygg 2 har inntrukket glass og solid skiltvegg.
- G454 13550 til 10702: 976 bil- og 1872 gamle vindustrekanter fjernet. G455 1712 til 1224: de to gamle fasadegrimeplanenes 488 trekanter fjernet; opprinnelig atlas og øvrige flater beholdes.
- SARO07 C10 med eget førermiljø og merking, to parkerte biler og 6 originale bygningsfamilier/28 deler/84 LOD-er. Servicebil, personlig bil og parkerte biler har egne roller. Lisensierte derivater holdes utenfor offentlig Git.
- 1400 selvtester, 10 målrettede pakkede forløp, 39 faste kamerastillinger og 44 bildehash. 9 kopierte Blender-originaler åpnet; 411 kildefiler hashkontrollert. Disse resultatene overstyrer ikke følgende avgrensninger:

- Faste kamerastillinger, native API-forløp og ordinære kjøre-/gangkontrollere er prøvd. Fysisk Xbox, full gjennomspilling og lydlytting er ikke verifisert.
- Den opprinnelige skycapture-shaderen utløste gjentatt Vulkan device-lost ved badekameraet. En avgrenset shadergren for refleksjonsopptak består isolert editor-A/B og dette pakkede utvalget. Selve shader-/driverårsaken er ikke bevist, og hele ruten er ikke stabilitetsmålt. Capture utelater fin panorama-/Melkevei-detalj og bruker HoleSky-kalibrert nattenergi; hele finite-hovedshaderen og kvaliteten på volumskyene er bevart.
- Full 110-scenariorunde og PSO-matrise er ikke kjørt. Ingen ny ytelsesgaranti.
- Det eksisterende varselet om raytracing-minne er fortsatt synlig. Renderer og minnebudsjett er ikke overstyrt globalt.
- Adgangsreglene til stengte bygg beholdes. Modellerte interiører gir ingen ny historieadgang.

- Levering: `/home/tom/Desktop/Signal47 modeller 25okt`. Bevis: `/home/tom/scratch-bm/Signal47-production-models`. PR155 er flettet i da4e2394 og samme testede M6 er installert: 40 identiske payloadfiler og 46 byteidentiske Saved-filer. Se PRODUCTION_MODELS.md og acceptance.json.

## SARO-ankomst og matstyring, 24. oktober 2026

- Sluttkilde `5ae3a3fc`, kandidat A3: markert besøksparkering ved (-12.8,-.60,14.3), flyttet og belyst monument ved (-7.3,-.60,15.5), gjennomsiktige vinduer med rammer. Asfalten er tett; kun tegnet overflate heves24mm for å dekke gammel vei, fysisk gulv er fortsatt-.60. Prosedyregress utelates fra lot og smale gangstriper.
- Personbil beholdes etter ankomst og diskreload. Ventende servicepickup er et snapshot av den samme originale bilen, med lokale synligheter, ISM-data og egne kopierte materialparametre. Gamle utrygge personbilposer migreres med arrivalParkingVersion=2; nye trygge poser beholdes.
- Mat: B Xbox / Q tastatur, D-pad/piltaster og A/Enter. Nedsetting krever nærliggende synlig originalflate; stow/discard er egne vedvarende tilstander uten konsumering/dialogflytting/tidshopp. Mindre og lavere pose. Originalt E/A-pickup og måltid bevares.
- Editor/pakking, 1400 selvtester, fem korte native pakkede kontroller og19 vurderte bilder PASS. Faktisk drive/controller/TripStep og gang til døren, disk-/verdenslastinger, syntetiske Slate-inputs. Fysisk Xbox-transport og menneskelig gjennomspilling/lytting er fortsatt UNVERIFIED. A1/A2 avvist med rålogger bevart. PR154 flettet i86eb0f8c, samme A3 installert med40 kontrollerte payloadhash og46 uendrede Saved-filer. Se SARO_ARRIVAL.md. Eksisterende rødt RT-minnevarsel80.499MiB/400MiB ved20%-terskel er synlig; ingen global grafikk- eller budsjettjustering.

## Highway-kontinuitet, 24. oktober 2026

- PR 153 flettet i acaa99d7. Samme testede H3 installert med 40 hashverifiserte payloadfiler, 46 uendrede Saved-filer og verifisert sikkerhetskopi.

- H3 fra 464078be: Roswell følger eksisterende historieport også i pendlerbilen; lydregisterets camelCase-motornavn brukes med reell lavpassfiltrering og trygg stopp/ny innstigning.
- Natur og én detaljert hovedstrømlinje deler kanonisk road-ramme; SARO-offset (8024, 0, -300), 200 m overlapp, 86 delte komponenter/4816 instanser. RoadFloras fem tomt eksporterte drawbarn måtte aktiveres etter runtimefylling. Ikke aktiver alle skjulte eksportnoder.
- Gamle wiremeshes deler gjerder og kraftledning. Tools/highwaypower/prepare_cpu.py må gjøre g684/g918 CPU-lesbare før cook; runtime fjerner bare høye ledningstriangler. 6 mm gjerdetrådkloner rasteriseres uten egen hardwareRT; nye stolper/kraftledninger beholder RT.
- 1392 selvtester, 7 fokuserte pakkede forløp, fire reelle områdeskifter og 8 vurderte bilder PASS. Natt bekrefter silhuetter, ikke fine bakkedetaljer. Eksisterende D2/H3 RT-varsel 83.107 MiB / 400 MiB ved 20% er urettet. HIGHWAY_FIXES.md/acceptance.json skiller testet kilde, råbevis og levering. Ingen full 110/PSO-runde, menneskelig lytting/input eller helruteytelse.
- Auditfixture må vente på World->bBusy før BeginCommute; ellers beholder åpningsturens ventende StartTrip sin opprinnelige posisjon.

## Mesa Diner, 24. oktober 2026

- Originalt ellevedelt Blenderkit i Tools/mesadiner: fasade/interiør, maskert støvparkering, neonskilt ved fasaden og tractor/reefer. Faktisk root/yaw/floor og originale handlinger/dynamiske objekter beholdes. Dør fysisk åpen; ny fysisk leaf/signfoot, atomisk komplett-kit-fallback.
- D2 fra9ff022a3:320 kilde-,47 native-,1383 selvtester og9 korte pakkede scenarioer PASS, ti sluttbilder vurdert. Kildescene og spillbilder leveres i skrivebordsmappen Signal47 Mesa Diner 24okt. MESA_DINER.md/acceptance.json har faktisk scope; PR152 flettet i61598105 og D2 installert med40 payloadhash/48 uendrede Saved-filer og verifisert backup.
- Ikke legg frontskirting gjennom åpen dør. ClearTimer inne i executing lambda kan destruere callbackens shared-captures; sett ferdigvakt før etterfølgende ticks. Plasser ikke native neonpunkter fåcm fra malt skiltflate; det ga hvite inverse-square-flekker.

## Trafostasjonens omgivelser og nærlys,24. oktober2026

- Tools/substationsite: tre egne kraftmaster på faktisk g873-bakke, eksakte ledere fra gamle busstubber, vedlikeholdsskur, tromler/verktøykasse og kontaktstein. Telefonlinje og eksisterende kulvert er bevart. Original Blend/PBR/3LOD; ingen ny falsk inngang eller oppdragsport.
- Kilde3f3687d5/kandidatP1:222 kilde-,29 native-,1383 selvtester og7 korte pakkede scenarioer PASS. Eksisterende inspeksjon/diskreload, nye modellvinkler og nær/far-lykt er kontrollert. SUBSTATION_SITE.md/acceptance.json har faktisk bevis; PR151 er flettet i c8c4de1f og kandidat P1 installert: 40 payloadhash, 48 uendrede Saved-filer og verifisert backup.
- Lommelyktens native spot kompenserer nå for nært synlig treff under3.5m, med jevn overgang og story/flicker uendret. Faktisk P1:1.472m→74.309cd,5.280m→420cd, begge ThreeIntensity64. Ikke mørklegg teksturene som lokal eksponeringsløsning.

## Caliche Junction Substation, 24. oktober 2026

- Originalt kit i Tools/substation: 10 deler, 3 LOD-er, redigerbar Blend og originale PBR-kart. Native root Three(7890,-1.10,536.20), yaw180, tilgjengelig kontrollfront utenfor stengt yard. Normal fotgjengermodus her er diner, selv om Props og gangdata er road.
- PR150 flettet i8bd33349, kandidatS2 installert: 40 payloadhash, 48 uendrede Saved-filer og verifisert backup.
- Kilde43ab2430 / kandidatS2: 304 kilde-,41 native-,1378 selvtester og seks korte pakkede kontroller PASS. Faktisk Walker-gange, gjerdestopp, øyetrace, journal/service/lampe og diskreload med samme pose/tid er kontrollert. SUBSTATION.md/acceptance.json har råforsøk og begrensninger.
- Walkable tester bare soner; collidere håndheves av Resolve. Test fysisk gange når et gjerde skal verifiseres. STATION01 world-origin er z8000, ikke0. Unngå ugyldige diagnostiske posisjoner. Nær lommelykt kan overeksponere nytt skap; lysbalanse gjenstår.

## Xbox-kontroller, 24. oktober 2026

- Xbox-kilde `3d016a07`, kandidat X4: 1373 selvtester, elleve native kernel/SDL/Slate-opptak i målte scopes og 16 målrettede regresjoner PASS. PR149 er flettet i fda74798 og kandidat X4 installert med 40 verifiserte payloadfiler og 48 uendrede Saved-filer. GAMEPAD.md har kontroller, bevis og uverifiserte handlinger.
- Tom ber om kortere testløp 24.10: velg nødvendige, målrettede kontroller etter endringens risiko. Den komplette 110-runden for Xbox ble stoppet etter 16 beståtte scenarioer; ikke kall den fullført. Bruk parallelle agenter til kode/gjennomgang; én tung jobb på maskinen består.

## Skyoppgaven sluttkontrollert, 24. oktober 2026

- Tom ber Codex fullføre Claudes volumetriske skyer etter limit. Skykode f55b760c/04d98231 er allerede integrert; ved overtakelsen gjenstod cache og levering på samlet natur/film/sky. Gammelt opptak under Signal47-sky har andre shader keys og kan ikke dokumentere ny pakke. SKY.md har faktisk sluttbevis.
- Ny samlet cache og sluttpakke er kontrollert: 84/84 opptaksbolker, 4329 SPC, 620 binære PSO-er, 285 runtime-graphics, 1328 selvtester, 110/110 unike scenarier/naturkontroller og 52 visuelt vurderte bilder. Lokalt flettet og installert med 40 payloadhash, 48 uendrede Saved-filer og backup. Skrivebordsmappen Signal47 skyer og lys har bildene. SKY.md har gjeldende bevis og målegrenser.
- Naturmulighetene Canyons of Utah, New Mexico-geologi, fotografisk fjernprojeksjon og Mesh Terrain er lagret som åpne vurderings-/prøvepunkter i todo.md, uten nedlasting eller migrering.

## Caliche natur, 24. oktober 2026

- Pakke/runtime/kilde/native/installasjonsbevis finnes i Tools/calichenature/acceptance.json og CALICHE_NATURE_PASS.md. screenshots.sh bruker starhole.log/png to ganger; første scenario er derfor tatt om med eget navn, uten å skjule rårapporten med avvist antallsport. Selve 110-scenariorunden bestod på 52,3 minutter.
- Tre originale Blenderkit i Tools/calichenature og calichestreet/backdrop, 35 597 deterministiske HISM-forekomster,36 mineralfliser,12 prototyper med tre LOD-er og 103 550 trekanter geologi. Native/kilde19ed326d, pakke de38da9747d9680c9788414688de9f641f09086bc1c936a38a8685619bad7ea8. Sky 04d98231 og core/film f9e1d4aa integrert. 110/110 full regresjon, 11 funksjonsprøver/1328 selvtester og 33 visuelt vurderte bilder PASS. Lokalt flettet i core 26dbb3df og installert med 40 payloadhash/51 uendrede Saved-filer. Se CALICHE_NATURE_PASS.md.
- Store terreng-UV-er trenger FP32: les faktisk render-FBX UV/RGBA tilbake, ikke bare importflagg. HISM-materialene må bruke used_with_instanced_static_meshes=True før rekompilering/lagring/cook; manglende SMU kan gi grå fallback selv med riktig materiale og teknisk mesh-PASS. Runtime avleser de faktiske kokte LOD-grensene og instanskomponentene. Beskyttet editorbool er fortsatt UNVERIFIED.
- Geologisk UV/Winding/veiklaring er egne porter. OldRoad-beskyttelsen kan lage urealistisk rampesilhuett selv med korrekt kollisjon/klaring; faktisk spillerbilde må avgjøre kunstnerisk form. CN5 beholder dette som kjent stilbegrensning, med lokale bergbrudd/erosjon.

## Filmfiltre, Codex 23. oktober 2026

- Tom bestilte alle filmuttrykkene med særlig vekt på Carpenter: Carpenter 35 mm, Desert 16 mm, Super 8, forbedret VHS og Clean. Implementasjon og faktisk teststatus føres i FILM_LOOKS.md. Nye High/Ultra-profiler får Carpenter; eksisterende eksplisitte valg beholdes.
- MPC_Web `Vhs` må forbli blend 0..1. Den brukes også i untonemapped-materialer, himmel og verdensliv, og SceneCapture setter den midlertidig til0. Nye enum `PictureMode` er separat. Styrke0 setter Vhs0 og tonepasset respekterer capture-gaten. Ikke legg mode2..4 i det gamle feltet.
- Generer bare filteret med S47_ONLY=picture etter editorbygg; film_tone.hlsl og MPC-tillegg må inn i cook. Bruk private Web-filer. Tilgjengelighet fryser korn og stopper flimring/registreringsuro/båndjitter. Ingen fps-lås er del av effektene.
- Sluttkode 8ba39556 inkluderer fellesgren 04517414 (nye antenner/radio). Sluttpakke i ~/scratch-bm/Signal47-film-looks/candidate-core: 1298 selvtester, ni funksjonsprøver, ti kamerakjøringer/36 bilder, MPC-readback, ekte settings-diskomstart og cache-GUID/213 graphics PASS. Samme binær og fem containere hashkontrollert mot alle kjøringer. Carpenter kostet +0,384 til +0,487 ms i korte High/1440p-vinduer; kontrollrom p95 var over 16,667 ms også i Clean. Claude har gjennomgått kode/bilder. Play.sh bruker egen player-profile; ingen installasjon eller publisering. FILM_LOOKS.md har UNVERIFIED for menneskelig temporal komfort, HDR/Deck og ny full-world-PSO-matrise.


Beslutninger og fallgruver. Dagbok hører hjemme i log.md.

## Caliche konseptpass, 24. oktober 2026

- Ni faktiske visuelle iterasjoner er levert og installert. Se CALICHE_CONCEPT_PASS.md og Signal47/Tools/calichestreet/acceptance.json for kilde, pakke, begrensninger og modellprioritet. Sluttpakke 6103dd97769d, native f63aba3a, C++/config 266f6fc2.
- Radio og butikk har slitt mineralpuss, malte skilt, treverk og variert fysisk innredning. Lokal grus/asfalt/jord har myke vertexalpha-kanter og rolige avbrutte spor. Sju fjerne mesaformer og lokale sky-/lysjusteringer gir mer dybde. Originale gulv, markeringer, ankre, klokker og samtlige 27 antenner bevares.
- Legacy FBX-reimport kunne beholde feil mesh-seksjonsbindinger etter materialsporendring. Egne genererte Caliche-/Station-mesher opprettes ferskt på samme stier; kontroller faktisk seksjon mot source used_materials, ikke bare materialspornavn. VertexColor REPLACE og eksplisitt GLB COLOR_0 alpha er verifisert. Dither-fades bruker teksturalpha ganger vertexalpha uten å endre original bakkekollisjon.
- Bakgrunnens gamle innbakte nattfjell skjules bare lokalt ved full Caliche-vekt og faktisk synlig ny bakgrunn. Alle opprinnelige synlighetsflagg gjenopprettes ved områdebytte eller fallback. Radioens punktorb er fjernet; innelys følger fysiske armaturer, fasadelys følger eksisterende døgngrense. R9-printkort og metrisk tre-UV endrer ikke pivoter eller adgang.
- 42 faktiske spillbilder visuelt gjennomgått, 110/110 komplett screenshots.sh, 11 runtimeprøver/1305 selvtestsjekker, fotopilot og tre horisontprøver PASS på samme uendrede pakke. 249 native kontroller, 239 filhasher og fire portable Blender-kilder PASS. 40 installerte payloadhasher og 51 uendrede Saved-filer. Dette er automatiske kamera-/API-prøver; menneskelig input, lytting, fullnatt og helruteytelse er fortsatt UNVERIFIED. Ingen ny ekstern publisering eller push/PR.
- Leveranse: ~/Desktop/Signal47 Caliche konseptpass. Fire originalblends med teksturer/eksporter, offline SPILLBILDER.html, LES_MEG_KONSEPTPASS.md og pakkehashbundne bevis. Tidligere antennekilde SARO_Array.blend og bensinstasjonsoriginal bevares på sine opprinnelige leveringsstier.

## Skumlere anomaly-pilot, Codex 23. oktober 2026

- Tom ba om sterkere uro etter integrert `ed061aca`. Gjeldende pass står i `ANOMALY_SCARIER.md`; førsteleveransens rapport er historikk. Korte observasjoner i verdensbildet, ingen ekstra fotokvittering, ingen «pappen lever»-stråmann. Bevis og plausible uavklarte valg ligger i film/notatbok.
- N02 forklarer kveldslampen, men ny nattfilm har et uavklart vindusomriss. A02 lagrer ankomstretning og en andre usett plassering nærmere veien; ny film har en ekstra form. P01 følger faktisk gange med nærmere printer, tier ved stans og blinker ON AIR i tomt studio. Disse tre designreglene bøyes bevisst for Toms «spenning foran regler»; harde klokker/kanon beholdes.
- `anomaly.DreadVersion=2` i save v3 bevarer film-ID/klokker, fase/retning og P01-slutt. Tidligere hørt P01 avsluttes ved migrering, uten reprise. Nye former er faktiske skjulte eksponeringsmesher med fri sikt og vellykket render, ikke bare metadatamerker. Tekst røper dem først etter fremkalling. Gamle foto beholdes.
- `AS47Web::AdoptNode` bevarer bare posisjon; bruk `FS47Obj::SetScale` for native deler som senere flyttes/roteres, ellers blir de enhetsstore. Nye cue-kilder og ambience-gate må eie egne lydnoder og resettes ved modal, stor scene, telefon, områdebytte, last og tittel. Ikke endre opprinnelige bussnivåer eller stemmer.
- Sluttspillkode `01d92bd4`, core206afd9b, pakke6 og 1293 selvtester PASS. Hele samlingen på siste pakke: åtte pilotscenarioer, High/VHS og Low, ekte gammel pilotmigrering, 7/7 historieregresjoner og to gamle kameramotiv. Tolv nye hovedbilder, før/etter og Low manuelt gjennomgått. Menneskelig input/lytting/frykt og fullnatt UNVERIFIED. Evidens i `~/scratch-bm/Signal47-anomaly-scarier`. Én tung jobb, ingen stopp av Tom og ingen installed-game-grace. Ingen push/PR/installasjon; Claude integrerer.
## Takeaway etter innkjøringen, Codex 24. oktober 2026
- Samlet verifisert pakkekilde `f83f3dc9` med core `92dc7a47`: 1298 selvtester, 33 vanlige bruksforløp/21 ferske matlastinger, begge biler, 48 inngangspasseringer, burgerrute og 3/3 korte regresjoner PASS. Seks pakkehasher uendret, bilder gjennomgått og midlertidig Content/OpenOrder ryddet. Nyere core `6614cd90` er ikke i pakken; Claude integrerer. Ingen push/PR/installasjon.
- `FS47Takeaway`, eid av `FS47Evening`, holder én porsjon pose/kaffe. Faste steder: `diner`, `desk`, `annex`, `hood`, `passenger`, `bench` (dinerens hagebenk, ikke kjemilabbens wetBench). Bilplasser følger bilens kanoniske pose. Ingen fri fysikk eller nye gangkollidere.
- Save får additivt `evening.takeaway` versjon 1 med `mode` og `spotId`. Pakken er sannhetskilde for kjøpt/utlevert/spist, også mellom UI-callback og neste Tick/autosave. Gammel utlevert mat blir båret; ukjent plass gir den tilbake til hånden. Spist mat kommer ikke tilbake.
- `EatTakeaway()` setter konsumering atomisk uten å endre dialogpeker, ventende svar eller dokumenter. Spisevalget finnes etter D01 ved skrivebord/annex, med plass til hele 720 sekunder før historiens neste reserverte grense. Tid går bare fremover. Sted, konsumering og tid committes før save; firesekunders fade er transient og gjentas ikke etter load. Ingen måltid under telefon, kritisk hendelse eller finale.
- Inaktiv matproxy må ha NoCollision, ikke bare tom Label: Inter.Pick stanser ellers ved første usynlige mål. AdoptNode normaliserer også meshskala, så målvolumets skala settes etter adopsjon. Holdt mat er uten kollisjon og skjules under kamera, lykt, modal/notatbok og kjøring.
- Den gamle workTable-queryen omsluttet matmålene ved 0,78 m. Den følger nå bordplaten som et 1 cm plan ved 0,775 m, med opprinnelig fotavtrykk/ID/arkivhandling og uendret gangkollider. Vanlig ray/fokus/bruk må fortsatt åpne arkivet. Dinerens gamle disktallerken skjules mens samme porsjon holdes; ingen dobbel servering.
- `-S47EveningTakeaway` og `-S47TakeawayEatPlaced` tester vanlig interaksjonskjede og ferske disk-/verdenslastinger. Modalskjermbilder må bruke sanntidsticker: Story.After står stille mens paneler pauser spillklokken. Rapport og faktisk sluttstatus: `EVENING_TAKEAWAY.md`; egen kandidat/evidens i `~/scratch-bm/Signal47-takeaway`. Menneskelig input, lytting og fullnatt merkes fortsatt UNVERIFIED.

## Åpningens biler og SARO-inngang, Codex 23. oktober 2026

- Pakket kilde `66d2b5a6`, core `04517414`, retter Toms tre åpningsfeil fra `ed061aca`. Rapport og reproduksjon: `EVENING_VEHICLE_FIXES.md`; kandidat/bevis: `~/scratch-bm/Signal47-opening-vehicles`. Nyere core-endringer er ikke en del av denne testpakken.
- Skjul HELE SARO-kjøreriggens Group under personbilbruk; selvstendige cab-/dørbarn blir ellers stående igjen. Wagonen bruker eget førerøye, komplett interiør og egen hengslet dør. Gjenopprett rigg og serviceførerøye ved ankomst.
- `fairheaven_sw84` er delt med props. Importens opt-in `keep_complete` bevarer standardmodellens 19 461 trekanter; den interaktive bilen bruker `_body` (18 244) og `_door_driver` (1 217). Regenerer/importer alle tre før cook; genererte Free-assets skal ikke committes.
- Kanonisk `SetParkPose` nuller pitch/roll og hjuldropp. Bare nøyaktig gammel originalpad repareres på load; andre serviceposer bevares. Utrygg/manglende personbilparkering bruker besøksplassen `(4.5,-0.62,14)`. Walkerens orienterte bilboks følger samme pose, slås av når skjult og slås opp på ID fordi dørbladene endrer indeksene.
- En lukket østdør kunne fjerne eneste gangsone under spilleren. Avvis manuell terskellukking mens spillerens fotavtrykk overlapper. Gammel ugyldig save må repareres FØR Flow-lasterens Walkable-port, ellers når den aldri reparasjonen. Åpne kun berørt terskel og bevar posisjon/klokke.
- `-S47EveningVehicles` og `-S47EveningEntrance` gir 1286 selvtester og 48 walkerpasseringer før/etter D01, parkert/flyttet, fire ferske disk-/verdenslastinger og gammel ugyldig terskelsave. Hel burgerrute og seks gamle historiehopp PASS på samme pakke. Bilder visuelt kontrollert, seks pakkehasher uendret. Dette er API-styrt normal bevegelse, ikke menneskelig tastaturtest. Content-lenker/OpenOrder ryddet; ingen installasjon, push eller PR.

## Anomaly-pilot, Codex 23. oktober 2026

- `codex/anomaly-pilot` implementerer N01/N02/A02/P01 på den flettede kveldstarten. Førsteleveransen er dokumentert i `ANOMALY_RUNTIME.md`; den senere skumlere runden står i `ANOMALY_SCARIER.md`. Senere kontor/trafo/VHS, bygningsløft og fullnatt er fortsatt egne leveranser.
- Gjenbruk kamera/instantalbum, docs/notatbok og authored N01-manus. Undersøkelser har separate referanser, eksponeringer, reviderbare vurderinger og fysisk oppfølging; `TownSeen` blir aldri automatisk et bevist funn. Tidlige paneler røper ikke P01 eller senere figurplassering.
- Save v3 får `anomaly` versjon1 og rutinekontrollpunkt før survey. Arkivets faktiske 17:10-JPEG lagres med saken som egen ID/url/clock, så senere programversjoner ikke bytter gamle referansebilder. Albumets `unavailable` følger en manglende original gjennom disk og verdenslasting; erstatningskort er aldri fotografisk bevis.
- Ett eksisterende pappobjekt har vindus-/studioposisjon med samme prisetikett og splitt. Vinduets høyde kommer fra faktisk byggproxy, ikke skrånende terreng. Siktkontroll følger albumets regler for synlige opake flater; usynlige hitboxer sperrer ikke foto. Ikke hardkod meshnumre.
- Tidlig kamera fullfører ikke B-12/mørkerom; rutinekjøring utløser ikke ch5. 04:55, Earl 04:00, P13 uten anomalikvote, historisk 1947 og 47 sekunders finale er bevart. Den første leveransen bøyde ingen designregel; den skumlere runden over gjør det uttrykkelig.
- Sluttspillkode 1d3baeb0 og separat testverktøy c6b4f023; core 609a9c6d innflettet. Pakke4, 1278 selvtester, hel pilot i High/VHS og Low/Clean, feilfilm, migrering, 7/7 kortregresjoner, direkte/burger med stemmer og eksisterende TV-opptak PASS. Fire sluttbilder visuelt kontrollert. Statisk Caliche 1440p p95 7,921 → 7,967 ms (+0,58 %), avgrenset av/på-kontroll. Bevis/proveniens i ~/scratch-bm/Signal47-anomaly-pilot og ANOMALY_RUNTIME.md.
- Én tung UE-prosess, 10 GiB og ingen installed-game-grace. Kilde, pakket kandidat og installert standard holdes adskilt. Ingen push/PR/installasjon, Claude integrerer. Menneskelig input, lytting, uro og fullnatt må merkes UNVERIFIED inntil faktisk prøvd.

## Kveldspakkens 109 stemmer, Codex 23. oktober 2026

- Produksjon a4626053 er koblet til runtime i b7faff35/6ade7a94/3345f01f/9d638a0b. Streng import må kjøres med original-WAV-er; den sjekker hash, målte tider, roller og alias før skriving. 109 originaler gir 110 cues, D61 deler første D01-fil. Generert Audio/SoundWave skal aldri committes.
- Host bruker receiverfilter og RadioBus, Dale phone/walkie/radiofilter, servitrisen panner ved sin verdensposisjon og SfxBus. Modal replikk begynner ved målt talestart og blir stående til Continue; radioens caption bruker målt start/slutt og lydklokke. Lagring starter gjeldende avbrutte replikk fra begynnelsen; manuspeker, betaling og svar beholdes.
- Ikke åpne samme aktive samtalepanel om igjen: UI-lukkecallback stopper stemmekøen. ShowDialogue er nå idempotent når panelet er åpent. Hele karakterpanelet er merket Unread for generell beskrivelsesoppleser, også ordre-/valgtekster, så en separat «No recording»-boks ikke vises over ekte karaktertale.
- Sluttpakke 9d638a0b PASS: 1258 selvtestsjekker, hel burgerrute med lyd (281,3 s kontroll, 36 ulike fullførte klipp med faktiske samples/RMS/peak), to disk-/verdenslastinger, radioavbrudd, lydpause/resume og kortregresjon 6/6. Fem rutebilder og D01-transkripsjon visuelt kontrollert. Rapport og reproduksjon i EVENING_AUDIO.md, evidens/kandidat i ~/scratch-bm/Signal47-evening-audio. Content-lenker/OpenOrder ryddet. Ingen push/PR/installasjon eller generert lyd i Git.
- D01_FOOD_002 (innledende «I») og D01_BRIEF_005 («observations») brukes, men krever Toms lytting. Automatisk nivåbevis er ikke menneskelig lytting. Senere kontor/trafo/VHS og fullnatt er fortsatt egne oppgaver. Claude integrerer.

## Kveldstart og tidslinje, Codex 23. oktober 2026

- `codex/evening-runtime` implementerer Toms trinn 1 og 2 i Unreal. Generert manusadapter, egen personbil med delt kjørerigg, valgfri diner/mat, engangsankomst 18:00 og D01 kun i telefon. Tre overleveringspapirer gis én gang; den eksisterende rack-/survey-rutinen fortsetter. Senere kontor/VHS/trafo er datadrevne porter. N01/N02/A02/P01 er siden implementert som egen anomaly-pilot, se statusen ovenfor.
- Nye saker har `v=3`, `timelineVersion=2`, residual 00:13:47 og kapittelminimum 02:20/03:20/04:10. Midnatt krysses på en sammenhengende akse. Gamle saker beholder eksplisitt legacy-versjon og 02:13-referanser. 1947-opptaket og finalen 05:29:00 til 05:29:47 er urørt. Hele gammel/ny-tabellen og komprimeringsreglene står i `EVENING_TIMELINE.md`.
- `printoutAt` er nå faktisk hentetid, separat fra solve/event. Gamle saves lagret aldri den tiden: en gjenopprettet utskrift merkes derfor TIME NOT RECORDED / reconstructed legacy copy. Fotografier omskrives ikke. Ikke oppgi en rekonstruert hentetid som original.
- Kveldsradioen har egen vert og separat mottakerkø. De 109 takesene finnes nå og importeres strengt med hash-/timingskontroll; se `EVENING_AUDIO.md`. Dale går gjennom receiverfilter, host gjennom RadioBus, servitrisen er posisjonell. Eksisterende countrymusikk har musikkrom mellom innslagene. Stemmeimporthjelpen produserer ingen stemmer. Nye rolleopptak virker uavhengig av generell Kinski-opplesning.
- Kveldshimmel og native sol bruker `Daylight()`; historiens `Dawn()` er fortsatt bare morgen. Stjernene holdes borte gjennom tidlig skumring. Destructor skal bare frigjøre egen lyd: `Evening::Stop()` berører levende verdensnoder og skal ikke kalles under sen UObject-purge.
- Bygg/sluttpakke og 1254 selvtester PASS. Fire pakkede åpningsruter, disk-/verdenslasting, native telefonbruk, gamle/nye kapittelhopp og legacy-migrering med bevart bildestempel/bytes er prøvd. Siste kode 3d321da1 inkluderer besøksparkering fri av annexet og ingen gammel 17:20-returtekst ved 18:00. Fem siste bilder og hel burgerrute er PASS. Resultater/proveniens/kandidat i `EVENING_RUNTIME.md`; menneskelig fullnatt/lytting/ytelse er UNVERIFIED. Ingen push/PR/installasjon. Claude integrerer; standardstarteren er bevart.


## Grafikkpass levert, 23. oktober 2026

- Grafikkens WIP 30d407be fortsetter 64d943e7/be7c89e5; core ae95b09a med Props og nyeste dokument-core 9d7571e2 er innflettet. Props-modul/config og installert spill er urørt. Se LIGHTING.md for siste faktisk målte kandidat og historiske avgrensninger.
- Bundlen er levert som S47_Signal47_SF_VULKAN_SM6.spc, 92 902 byte, 4215 stabile PSO-er. Full 38 × 6-matrise med én cook/frosne nøkler er komplett. Sluttcook gir 599 PSO-er: 222 graphics, 310 compute, 67 RT. Pakket runtime åpner samme GUID og behandler 222 bundled graphics; ni cachebeviskontroller PASS. Nye render-/material-/Props-baner krever ny cachevurdering/opptak.
- Alle 9243 cooked render-/asset-/shaderfiler er byteidentiske mot opptaket. Global.shk er byteidentisk; Signal47.shk har ulik serialiseringsrekkefølge/filhash men nøyaktig samme 21 184 logiske tupler/outputhasher. Ikke kall metadatafilene byteidentiske. Frosne nøkler og recordings er urørt; se static/final-cook-equivalence.json.
- S47Theme-lastefasen dekker kompilering/oppvarming, med spill-/testklokker ved READY. Sluttpakke kald READY 11,337 s, varm 5,594 s; sluttregresjon bekrefter tittel/pause/notatbok/kjøring/THE EVENT og cap30 med 285 intervaller på 33,333457 ms i snitt. Første 10 synlige sekunder 1111/1095 rammer, maks 31,585/31,888 ms, ingen over 33,333 ms. Før kald 455 rammer/maks 1296,925 ms. Nullpunkter er verdensstart og HUD/READY-proxy, ikke OS-start/fysisk presentasjon.
- 118 materialprofiler, seks glass og fire CRT-valg må regenereres med S47_ONLY=materials før integrasjon av eldre generert Content. Masters-only reparerer grafene og hopper over MI-parametrene. StaticSwitchParameter krever True/False; streng cookkontroll avviser standardmaterialfallback selv ved UAT exit0. Web/Audio/Licensed/Free-assets committes ikke.
- Seks GLB-spor med én kildematerialoppføring må bindes på rack/diffusere. Native DP-80/CB/3278 bruker EmissiveFactor/Strength, Web-materialer Brightness/EmissivePower. Gate emisjon og bevar PBR/maps. V5/V9 er supplert av sluttpakkens komplette sju indikatorprøver og 36 fasebilder, inklusive selvtest (1220) og faktisk terminalfront av/på. Matrisens gamle dark-bilder er ikke strømbevis; after/final-state-correction og sluttfasetesten er korrekt retest.
- Alle 20 natt + 20 daggrybilder er visuelt kontrollert; 40 par og originalhash/galleri ligger i compare/index.html. Etterbilder har innflettede Props; de er ikke isolert lysforskjell. PreviousNight-kjøring og alle åtte lengre High-prøver er PASS; siste seks regresjoner og begge identisk-geometri fyllpar er også PASS. Korte High-prøver viser 90,1 fps eller mer i gjennomsnitt, men p95 opptil 21,772 ms; lengre natt/daggry viser 93,81–106,73 fps og maks p95 18,142 ms, så det er ikke låst 60 fps for hvert intervall.
- Statisk diagnostisk tittel må ryddes før shutdown. V2s to SIG11 er forkastet; v3s tidligere feilsekvens består også med logPSO. Én tung Unreal-jobb, minst 10 GiB og ingen stopp av installert spill. Tom avsluttet selv før 11:06, deretter kjørte matrisen. Midlertidige Content/Audio, Content/Licensed og Content/Free-lenker er fjernet med kildemappene bevart; OpenOrder-logger er gjenopprettet. Kilde og bundle er levert i 684bc142 på port-gfx.

## Gameplayretning (2026-10-23)

- Tom sluttet seg til aktiv anomaly spotting, mer praktisk bruk av Caliche og gradvis UFO-/alienopptrapping, og ba om en utdypet plan. Se `ANOMALY_PLAN.md`. Detaljene er anbefalt design; N01/N02/A02/P01 er siden implementert på `codex/anomaly-pilot`, se ANOMALY_RUNTIME.md.
- Tom ba senere om tidligere start, ny trafostasjon i ørkenen og bedre bybygninger. Planen fikk 18:00-start 13. april og residual 00:13:47 den 14. april, i stedet for 02:13:47. Tidslinjen er nå implementert på `codex/evening-runtime`; se den nyere statusen ovenfor. Finalen 05:29 beholdes, og historiske 1947-klokkeslett skal ikke masseflyttes.
- Tom valgte deretter spillbar innkjøring før skiftet: lystig radio gjennom Caliche, valgfritt matstopp i dineren, videre til SARO der skiftet begynner 18:00. Innledningen har egen fase uten løpende skiftklokke, slik at matstopp ikke gir tidstilbakesprang. Tidlig dinerbesøk aktiverer ikke ch5/P13/Ward. Reisekjøretøy og servicepickup holdes adskilt.
- N01s grunnmateriale finnes nå i overleveringsmappen på SARO; innkjøring blir ikke en obligatorisk papirjakt. Materialet brukes mot utskriften etter midnatt. Dokumentert byreferanse må finnes også når matstopp hoppes over. Earl har Night Line 00:00–04:00, ikke kveldsvakt; ny datoakse, radiosending, butikker, NPC-er og totimerskravet må tåle midnatt.
- Tom presiserte at Kinski/Dale KUN er på telefon, radio eller walkie-talkie. Overleveringen 18:00 er en telefon etter at Dale har dratt; ingen fysisk Dale-figur eller Kinski-forteller på VHS. Egen rollebasert Dale-dialog må skilles fra generell opplesning. Ward beholder rollen som vakthavende.
- Manuspakken i `Narrative/EveningShift` har 61 scener, 45 valg, 10 dokumenter, 39 UI-tekster og et 72 sekunders VHS-manus. 110 talecues bruker 109 unike takes fordi et fast Dale-fragment gjenbruker første overleveringslinje. Engelske JSON-moduler er sannhetskilde, med generert lesemanus, stemme-CSV, opptaksmanifest og undertekstgrunnlag. Åpningens runtime er integrert på `codex/evening-runtime`; senere faser er porter. De 109 takesene er produsert og koblet til runtime; VHS står pending.
- `build_pack.py --check` validerer referanser, flagg, kanaler, medietidsdekning og opptaksalias. 14 offline forfatterruter/porter er prøvd, inklusive matvalg, kontor uten VHS og faktiske kontrollkrav. Dette er ikke ekte kjøring eller menneskelig spilltest. Ingen ny stemmetjeneste er installert.
- Ny D02 bruker eksisterende låste Operations office-dør: en fysisk, merket reservenøkkel finnes under arkivundersøkelsen. Kontoret viser Dales hemmelige UFO-/alienresearch gjennom utklipp, filer/kontaktark og faktisk bevegelig VHS/TV med lyd. Ett feltopptak gir konkret kontroll ved trafostasjonen. Bevar Noras nøkkel, den brede 1947-avisen og P13. Dale er ikke automatisk skurk, allvitende eller et svarende avtrykk.
- Gatetelefonens to hendelser og dinertelefonens Ward-samtale har forskjellige krav. A gjennom byen er ikke C ved finalen. Nye byankomster må ikke starte ch5 eller den gamle veien.
- Første pilot bruker N01/N02/A02/P01 med kveldens referanser. All Night anbefales fra 04:10, slik at det er mer tid i byen og mulighet for lysbølgen før 04:55. Gamle kapittelminimum og underhendelser må retimes samlet.
- Ny trafostasjon, Caliche Junction Substation, skal ha en vanlig feil/kontroll på første tur (T01) og en annerledes oppfølging senere (T02). Den er ikke STATION 01, og SAROs K3 blir ikke byens strømnettbryter.
- Alle bybygningers utvendige skall skal forbedres, med første sammenhengende gatebit ved bensinstasjon/suvenir/studio. Diner, sheriff og vaskeri følger med nødvendige rom; øvrige bygg får samme utvendige kvalitet. Bevar egne Blender-kilder, ankre og interaksjoner. Bruk eksisterende konseptbilder som visuell retning; material-/lyspasset alene oppfyller ikke bygningsbestillingen.
- Pilotens 20–30 minutter gjelder avgrenset byinnhold med bevis gjennom lagring og tydelig testkutt. Kveldsoverlevering, trafo og mellomkapitler kommer i tillegg og må måles. Se ANOMALY_RUNTIME.md for pilotens senere runtimekontroll. Menneskelig opplevelse og full spilletid er fortsatt UNVERIFIED.


## Native belysning (2026-10-22)

- Videre belysning er bare Unreal. `S47Lighting.cpp` gjør eksportens lampetilstand om til punktlys og spotter. Historienes SetFlood-API er beholdt; materialenes flood, hemisphere, FogExp2, additive lyskjegler og vindusprojeksjoner er av. Ingen Web/Unreal-bryter. Eldre lysnotater nedenfor beskriver porten før denne endringen.
- Lumen bruker Vulkan hardware ray tracing på RTX 3080 når tilgjengelig. Måne, skylight, levende daggry, native volumetrisk tåke og skygger fra lommelykt/billykter erstatter fylltermene. Førerhusets gamle himmelfyll er fjernet; instrumentgløden er svak. Fast eksponering, ACES og VHS/Clean beholdes.
- `S47_ONLY=materials` i import_web.py regenererer Base og Mat uten å skrive Tex, Geo, art eller WebData. Kreves for eldre generert Content. NativeLighting har asset-default 0 for delte historiske checkouter, men denne runtime setter alltid 1. Bruk private materialkopier ved parallelle jobber.
- Low bruker direkte lys, High/Ultra Lumen og volumetri. High/Low bruker 60 % intern oppløsning med TSR; Ultra 100 %. Se `LIGHTING.md` for faktisk ytelse og sammenligningsbilder, ikke trekk fps-konklusjoner fra kvalitetsnavnet.
- Rettelsesrundens High/1440p med TSR fra 60 %: rom 13,619 ms / 73,4 fps, gård 10,466 ms / 95,5 fps, p95 15,330/11,527 ms. Ingen andre tunge prosesser i målevinduet. Low/Clean 90,0 fps i rommet, Ultra 39,2 fps i gården. Faste editorutsnitt; pakket bygg og bevegelse gjenstår. Se LIGHTING.md.
- Før tung kjøring må også prosessnavnet `Signal47` sjekkes. Den installerte Linux-binærfila heter dette, ikke nødvendigvis Signal47-Linux-Shipping. En slik prosess gjorde første ytelsespass ugyldig selv om UnrealEditor-kontrollen var tom.
- Kopiert UBT Intermediate inneholder absolutte stier. Lag ny Intermediate i et worktree, og kontroller kompilatorens response-filer. `Scripts/build-editor.sh -NoUBA` brukes her etter UBA-feil på PCH-rename gjennom symlink. Byggskriptet videresender valgfrie flagg.

- Taklys i SARO ligger i mesh-diffuserne, med rectlys og PBR-emissive fra samme tilstand. Rommets gamle PointLights er tilstandsdrivere uten bAffectsWorld. Fire CRT-er og øvrige registrerte skjermer har egne grønne rectlys; HUD-ens Ops power-kall overstyrer ikke SARO mørkt, fordi faktisk Brightness også leser stasjonsstrømmen.
- GLB-bokser kan ha slot0..5 selv når N.Mats har én oppføring. Lysrørets underside er slot3. FixtureLighting binder alle diffuserflater til samme MID og følger NodeMids sine senere kopier; ikke oppdater bare materiale0 og la gamle emissive-flater stå igjen.
- Annex.flickerTube ligger ved pool3, mens portens FlickerFlood er4. Den fysiske koblingen remapper dette til det faktiske røret. Endre ikke historietidsplanen for å rette lyskilden.
- Arrayets første fem pools ligger ved Y3, men armaturene ved Y6,5. Native spotter står på armaturene og sikter mot skålene. Seks andre eksisterende feltarmaturer bruker sine eksporterte glødepunkter; de deler arrayets strøm/rødpuls. Ingen nye modeller.

## Oppsett (2026-10-21)

- Prosjektet heter Signal47 og ligger i unreal/Signal47. UE 5.8.4 (binærutgaven for Linux) i ~/.local/opt/UnrealEngine-5.8.4, Vulkan SM6, Lumen i programvare, Nanite av for de importerte modellene.
- Pakkede bygg legges utenfor repoet, i ~/scratch-bm/Signal47-build, så Artifacts-regelen i rotens .gitignore ikke berøres.
- Akser: X nord, Y øst, Z opp, cm. Nettversjonens (x, y, z) i meter er (Y, Z, -X) ganger 100. Vinduet i nettversjonen ligger ved z -4.65 (nord), altså X +465 her.
- Rommet er 1200 (Y) x 900 (X) x 300 cm, vindu fra Z 95 til 275 langs hele nordveggen med sprosser hver 180 cm, dør mot sør ved Y -290 til -190 og dør mot øst ved X -210 til -110.
- Modellene importeres med det gamle FBX-importeren (FbxImportUI), uten materialer; hvert materialspor får en MaterialInstanceConstant av M_S47Flat med fargen fra Unity-materialfilen med samme navn (_BaseColor, 1 - _Smoothness, _Metallic, _EmissionColor).
- Fonten VT323 finnes bare som woff2 i web/. Konvertering krever fontTools med brotli, som ikke er installert. Reserve: JetBrains Mono fra THE BLACK MAP (OFL).
- Stjernehimmelen er en tekstur laget ved oppstart (UTexture2D::CreateTransient) på innsiden av en kule med skala 400; fiktiv himmel uten katalog.
- Eksponeringen er manuell (bias 0, fysisk kamera av) så natta blir mørk med vilje. Lamper i candela: taklys 7 til 9, skrivebordslampe 3, gårdslampe 14. Månen er et retningslys på 0,9 lux.

## Prologen (2026-10-21)

- AS47Story er en direkte port av Prologue.ts. Spilltid (GameTime) stopper mens et papir, konsollen, terminalen eller notatboken er åpen (HUD-en setter Story->SetPaused). Tidsplanen (After) går på spilltid.
- Historien når tingene i rommet bare gjennom AS47Saro sine settere og PropLocation(id). Nye ting får Tag(actor, id) og en linje i Label() og Use() i S47Story.
- De fire CRT-ene er render targets på 512 x 384 som HUD-en tegner tolv ganger i sekundet med FWidgetRenderer (SS47Crt). Skjermflaten er en tynn boks med M_S47Screen (Glow 0 når skjermen er av).
- Lyder slås opp ved navn i /Game/Licensed/Sfx, så /Game/Audio/Sfx, så /Game/Audio/Synth. Mangler en lyd, logges «no sound named» én gang og spillet går videre.
- (2026-10-22) Lyden er nå nettversjonens Web Audio portert til C++ (S47WebAudio): én kontekst på 48 kHz, spilt av én USynthComponent, med egen panorering mot lytteren (spillerens øye, satt i US47Audio::Tick). Opptakene dekodes fra WAV i Content/Audio/Raw og Content/Licensed/Raw (index.json). Et filnavn fra prologen (thud_soft) går gjennom grafen når opptaket finnes der, ellers som før til lydassetene.
- Lydmotoren følger Chromium der nettleseren avviker fra spesifikasjonen, fordi det er slik nettversjonen høres: en løkke gjennom en DelayNode er én kvant (128 rammer) lengre enn forsinkelsen, firkant og sagtann har toppen 1 etter Gibbs (nivå 0,848), og kompressoren er Chromiums DynamicsCompressorKernel. Kryssjekket mot Chromiums OfflineAudioContext: 1e-4 eller bedre på konvolutter, biquader, panorering, avspilling, kompressor og kam.
- Lyd-API-et tar låsen per kall; hold FS47AudioContext::FBatch over en hel graf. Bygg lange buffere (amRadio-melodien) før låsen tas, så lydtråden ikke venter.
- De kjøpte glb-modellene importeres av Interchange til /Game/Licensed/Models/<pakke>/StaticMeshes/<mesh> (crt_terminal har crt_terminal og crt_keyboard).
- Et medlem og en konstant kan ikke hete det samme (RelayFirst): clang tar medlemsfunksjonen.

## Tegningene og kapittel 2 (2026-10-22)

- Lerretstegningene (drawings.ts) tegnes med Slate i et render target og leses tilbake: `FWidgetRenderer(false)` (uten gammatrinnet) og et vanlig 8-bits mål, fargene med `FLinearColor::FromSRGBColor`. Da kommer lerretets bytes ut som de er og blandes som lerretet blander. Med `FWidgetRenderer(true)` blir fargene sRGB-kodet to ganger (papiret #efe8d6 ble #f8f5ec).
- Slates kantutjevnede linjer toner ut over to piksler, lerretets over én. Tegningene tegnes derfor i dobbel størrelse og midles ned.
- Nettversjonens Oswald er ett statisk snitt, Medium (500), oppgitt for vektene 200 til 700 (core/fonts.ts), så nettleseren tegner all Oswald i Medium, også titlene som ber om 600. `Content/Fonts/Oswald-Regular.ttf` er en variabel font som Slate tegner i standardsnittet 400. `Oswald-Medium.ttf` er 500-snittet av den, laget med fontTools (`varLib.instancer`, uv-venv i scratch); tegningene bruker den. `S47Style::Display` bruker fortsatt Regular.
- Redigeringsprogrammet kan kjøre spillet uten vindu og uten den genererte verdenen: `UnrealEditor Signal47.uproject -game -RenderOffscreen -nosound -S47SmokeTest -S47SelfTest`. Det holder for selvtesten og for logikk som ikke trenger verdenen.
- Lydnavnene i kapittel 2 er nettversjonens (`phoneRing`, `click`) gjennom `Sfx` og `LoopSfx`; stand-ins i S47AudioWeb gir `PhoneCall` unset, så tekstingen går, og `Ringback` regner ennå ikke med starten sin.
## Lagring og flyt (2026-10-22)

- Lagringene ligger i Saved/SaveGames: save-<sak>-<auto|manual>-<plass>.json med kortets felt og `state` (GameState v2 som i nettversjonen), bildene i photos/<nøkkel>.txt og referert som `idb:<nøkkel>` som i nettversjonen, active.json er saken som ble spilt sist, settings.json er innstillingene (main.ts store).
- Det som ennå ikke er portert (bilen, lommelykten, bortføringen, albumet, kurven, Caliche, ns.ln, ns.array, ns.hondo, kapitler uten C++-port) leses fra lagringen som åpnes og skrives tilbake uendret (`US47Flow::Carry`).
- Pausemenyen stopper verden med SetGamePaused, og lyden pauses med den. Verdenens tidtakere står da stille, så røyktestens tidtakere går på FTSTicker.
- Ethvert -S47-flagg unntatt -S47Picture og -S47Title starter rett i spill som før; uten flagg starter spillet på tittelen.
- I UE 5.8 har FJsonObject::Values nøkler av typen UE::TSharedString: `const auto&` i løkker, `FString(*Key)`.
- AActor har en virtuell Reset(); `AS47Story::Reset` er Prologue.reset og overstyrer den.
- Uten den genererte verdenen kan prosjektet prøves med UnrealEditor <uproject> -game -RenderOffscreen (tom verden): menyene, notatboken og lagringen virker.
## Kjøringen (2026-10-22)

- Kjøringen går i AS47Walker sin tick (CameraOverride → US47World::Frame), så bilen og kameraet flyttes i samme bilde; Unreal har ingen fast rekkefølge mellom gåeren og HUD-en.
- Bilens gruppe roteres i Euler YXZ (stigning, kurs, krengning) som i Drive.ts; FS47Truck holder Position og Rotation selv, så World.ts sin `rotation.y` er kursen.
- Et render target tegnet her trenger SetMap(rt, true) (radene motsatt vei av de eksporterte bildene), som CRT-ene.
- clang godtar ikke en struct med standardverdier inne i en UCLASS som standardargument; FS47TripOpts ligger derfor utenfor US47World.

## Radio og musikk (2026-10-22)

- `S47Radio` / `S47TripRadio` / `S47RadioVoice` / `S47Score` er portene av de tilsvarende TypeScript-filene. Innkoblingen er i Main, World og Flow. Prologens bordradio bruker fortsatt sin egen `amRadio`, som i nettversjonen. Tittellåten bruker lydmodulens `music`-løkke og musikkbussen, utenom Score.
- Lange opptak ligger som PCM16 WAV i `Audio/Raw/media/{music,radio}` og går gjennom FS47MediaSource inn i den samme Web Audio-grafen. Ikke legg dem i Audio.Buf: musikken alene ville brukt omtrent 1,4 GiB som float. Kilden leser 32768 rammer per bit, opptil fire biter, med videre innlesing på bakgrunnstråd. Hold aldri en FBatch rundt Open/Seek eller innlasting av stemmeklipp.
- `audio-raw.py --recordings-only` oppdaterer media, de fire RadioVoice-bankene og katalogene uten å erstatte eksisterende sfx-/telefon-/kjøpt-lydindeks. Kjørt mot hovedkopien: 22 lange opptak og 259 stemmeklipp. Alt under Audio/Raw er allerede staged som UFS.
- `M->TownSeen` inneholder blant annet radioens `ev:boy`, `ln:reel` og `tr:<id>` og lagres i nettversjonens `town`. Øvrige nattsystemer må dele dette settet. `Score->Held` har navngitte funksjoner for deres tilstand inntil de porteres og kobles inn.
- Radioens tekst bruker lydkontekstens klokke i `UI->Toast`, slik at en pause holder både innspilte ord og tekst. Selektiv `Disconnect(destination)` slipper bare den ene mottakeren.
- `-S47Radio=caliche|roswell|far` plasserer en testmottaker ved kameraet gjennom den vanlige grafen og logger stasjon, frekvens, segment og lyttere. `screenshots.sh` har fått en radio-kjøring. Se `RADIO_PORT.md` for tester, native avvik og integrasjonsarbeid.

## Caliche, veien og motellets natt (2026-10-22)

- `US47TownLayer` samler main.ts-koblingene til Town/TownEvents, Roadside, Herd/Pasture, NinthModel/NinthCow, MotelTv, MorningHolds/DinerCooler, Cutouts, Caliche og Diner. Modulene deler `Main.TownSeen`; TV-bildet ligger i `GameState.motelTv`. Trinn og restore er delt inn slik at telefonene og SameThread beholder main.ts-rekkefølgen.
- Caliches stemmer bruker `RecordedDialogue`, `NpcDialogue` og `EarlDialogue` gjennom RadioVoice og den eksisterende Web Audio-konteksten. LostNights må koble `Caliche.Reel`; kapittel fem bruker `TownLayer.Diner`. Kapittel tres `FreeShot` kobles når kapittelheaderen er tilgjengelig.
- Byens bilkollider må oppdateres både i AS47Web og i kjørekontrollerens verdikopi (`World.TownCarMoved`). Direkte områdehopp trenger Player.floor-funksjonen før Place; ellers kan en lav plassering fra SARO bli liggende under Caliches terreng.
- Stopp lydgrafer i native destruktorer uten å gå gjennom Main/Audio/Web. Under editorens avslutning kan Webs nodetabell allerede være frigitt. Roadside og TV har derfor egne nodebaserte stopp i destruktorene.
- Ny eksport/import kreves for `world.town.runtime.*`, `roadside:car`, delte lysplasser og herdets beamLit-materialparametre. Ikke erstatt disse med håndbygd geometri. `pointScreen` er ennå bare eksportmetadata. Se TOWN_PORT.md for nøyaktige grenser og testflagg.
## Verdenens eget liv (2026-10-22)

- `US47WorldLife::Step` kalles sist i `US47Main::Step`, i main.ts sin rekkefølge for steg 13: himmelen, lampene, de delte lysene, tåkefargen, horisonten, så områdenes oppdateringer.
- Himmelens gruppe tegnes ti ganger så langt ut som i three (kuppelen på 30 km); tonepassets tåke stopper ved 25 km (SKY_CUT). Gjennomsiktige ting i himmelen sorteres med prioritet (stjerner -4, komet -3, stjerneskudd -2, bånd -1).
- Lys som legges oppå bildet (kjegler, vinduslys, møll, støv) ganges med DEFOG = 1 / (1 - tåka bak), fordi tonepasset tåkelegger hele bildet etter dybden bak.
- Det three tegner med fog: false og som er ugjennomsiktig, tegnes inn i custom depth (r.CustomDepth=3): stencil 1 for de nære mesaene (horizon.ts hazed: bare daggryets dis, MPC HorizonHazeK), 2 for resten (SAROs dishes sett fra den gamle veien). M_WebTone og DEFOG leser dem.
- VertexColor-uttrykket har ingen navngitte utganger: koble med '' (RGB). Et navn den ikke har (`RGB`, `A`) gir en ukoblet inngang uten feilmelding, altså hvitt. Alfa leses i en Custom-node (`Parameters.VertexColor.a`). En VectorParameter i en Custom-inngang gir float3; bruk en float4-node når HLSL-en venter fire verdier.
- `UInstancedStaticMeshComponent::SetCustomData(Start, End, ...)`: End er den siste indeksen, ikke én forbi.
- ISM med per-instans-data i et materiale: `NumCustomDataFloats` før instansene, og PerInstanceCustomData i pikselshaderen virker.
- Floraens tilfeldighet (mulberry32, hash4, vnoise) må regnes i uint32 med nettversjonens overflyt; selvtesten sammenligner hver rute med `life.tests` i manifestet.
- `-ExecCmds="r.SetRes 1600x900w"` gir -RenderOffscreen-kjøringer i 1600 x 900 (ellers 888 x 500), så de kan legges ved siden av nettversjonens bilder.
- Python-skript i et skrapområde må ikke hete som standardmoduler (`inspect.py` stoppet Playwright).

## Kapittel seks og slutten (2026-10-22)

- `US47Chapter6` er Chapter6.ts: uspart kjøring, siste mila med CB og avstandsklokke, THE EVENT på 47 sekunder, nøkkel/dør, figuren og kamerauttrekket. `ClockAfter` har ch6 sist, og Away-krokene ivaretar main.ts sin E/Q-rekkefølge. `World.OldControl.Camera` går gjennom eksisterende `Walker.CameraOverride`; Walker endres ikke.
- `US47Flow::Finale` stanser 3D-spillet og bruker `SS47Ending` sine sanntids-callbacks til tittel, kreditering, fotografi og `ToTitle`. Ch6 har ingen SaveState eller checkpoint. Siste dinerlagring skal være Fortsett; den virkelige koblingen fra kapittel fem må integreres når dens port finnes.
- `AS47Web` har en merket sluttblokk med faktisk tåkeoverstyring og daggry-/solverdier med hooks. Verdensporten må binde hooks eller lese getters. Et statisk `M_WebSky`-panorama kan ikke tegne soloppgangen eller høydeavhengig daggry; ikke kall den visuelle delen ferdig med bare disse setterene.
- OldRoad-konstruktøren legger figurens sju glødepunkter sist i `world.oldRoad.glow.points`. De gjenbrukes direkte; ingen ny veigeometri eller eksport er nødvendig for kapitlet. `US47Away::ShowFrame` har valgfri gruppeopacity for Ending, mens bortføringen beholder sitt svarte underlag.
- Integrasjonskontrakter og faktisk testgrunnlag står i `CH6_PORT.md`.

## Retning og lokale TV-klipp (2026-10-22)

- Tom har bestemt at videre arbeid kun gjelder Unreal. Web og Unity er referanser.
- Unarius-MP4-er ligger i `/home/tom/Desktop/Unarius/`. Film skal bevege seg på TV-ene. Lokal kandidat, status og videreføring står i `TV_PROGRAMME.md`.

## Egen alien, 22. oktober 2026

- Ny figur med Blender-kilde, 2K PBR, rigg, idle og importprøvde UE 5.8.4-assets ligger i `/home/tom/Desktop/Alien - Egen modell/`. Se README der for import og rapporter. Gammel `/home/tom/Desktop/Alien/` er urørt referanse. Ny modell er klar som ressurs; integrasjon i pakket spill gjenstår.

## KELVORA-rekvisitter, 22. oktober 2026

- Egen CRT-TV og slitt radio i `/home/tom/Desktop/Retro TV og radio/`, med PBR, FBX/GLB, LOD1/2 og importprøvde Unreal-assets i `Unreal/Content/Kelvora/`. TV-skjerm er materialplass 1 med eksisterende filmteksturparametre. Assetene er ferdige; plassering og pakket film-/radioprøve gjenstår hos integrator.

## KELVORA plassert 22. oktober 2026

- `S47RetroProps.cpp` bygger egne TV-er ved `world.motel.tv.group` og `world.room6.objs.tv`, radio ved `room.objs.radio`. `BuildRetroProps` kjøres etter BuildRoom; nye barn følger områdenes synlighet via AdoptNode og gamle bruk-ID-er. Skjermenes egne MID-er registreres på de eksisterende materialhåndtakene, med startverdier fra gamle MID-er.
- FBX-screen UV0 er snudd i V av importøren. Eget unlit skjermmateriale inverterer V før EmU/EmV, fordi de eksisterende canvas-tegnerne setter EmV=(0,-1,1). Ikke fjern dette eller innfør ekstra film-vending. CRT-kildens ringpolygoner hadde motsatt retning av senteret; alle er nå vendt ut og testet i alle LOD-er. Unlit unngår at TV-ens eget punktlys dekker filmen med en hvit refleks. Piloten sjekker RGB, ikke alfa.
- `Content/Kelvora/Office` og `office-patches.json` klipper kun gamle TV-deler fra originalkontoret; g-ID-ene må regenereres ved ny verdenseksport. Kilder og importløype er beskrevet i Tools/kelvora/README.md. Originale Blender-kilder på Desktop/Retro TV og radio er oppdatert.
- Kandidat `scratch-bm/Signal47-kelvora/build` er installert i Games/Signal47, hashverifisert med 27 save/config-filer bevart. 1218 selvtester, ni scenarioer; siste materialversjon er også prøvd på begge TV-er og med av/på. Fullt 47-sekundersopptak/lagring bestod før siste V-only shaderendring. Ingen menneskelig lytting eller hel gjennomspilling i denne økten.
## Utelys og fotolab, 22. oktober 2026

- `port-light` er videreført etter at forrige lys ble integrert/pakket. Ny leveranse tar inn `unreal-port-core` til `3b44956e`, med gratismodellimport, Claudes mørkeromsregel og brettrettelser.
- `S47ExteriorLighting.cpp`: 30 korte, fysiske spotter fra eksisterende armaturer; `S47LabLighting.cpp`: adresserbar `Light("lab.normal"/"lab.safelight"/"lab.enlarger")`, intensitet 0 til 1, og fysisk normal-/rødt-/linse-/varsellys. `US47Chapter1::Darkroom` kopler renderdriverne ved faseskifte; ingen ny film-/dørregel i dette lyspasset.
- Bygg og 1218 selvtester OK, før/etter fra ni kameraer, labmoduser/halvstrøm/kutt/tilbake og faktisk kontaktkopitilstand prøvd. Rapport/API/bilder: `LIGHTING.md`, bevis i `~/scratch-bm/Signal47-lighting-2026-10-22/revision3`. Ny pakke, full fremkalling med ekte input og nye High/1440p-tider gjenstår hos integrator. Ingen endring av installert spill.


## Radiostasjon og array, 2026-10-23 kl. 23:53

Original Blender-radiostasjon 1420 AM med analogt studio og separat lobby følger konseptarket. VLA-lignende antenne har fagverk, feedarmer, serviceplattform og tre bevegelige deler med tre LOD-er. Alle 27 antenner er erstattet. Kildene og native UE-assets er lokalt flettet inn i unreal-port-core fra codex/radio-array-assets. Se RADIO_ARRAY.md og Signal47/Tools/radioarray/README.md.

Ni antenne-FBX-roundtrips, stasjonens 32 eksportkontroller og UE-importens 68 kontroller PASS. Integrasjonspakken fra `66f41e22` bestod 1278 selvtester, antennebevegelse, lukket dør, åpne/gå 6,1 m inn og buzzer. Gangsonens terskelgap ble rettet. Visuell gjennomgang fant gammelt studiobilde som frittstående flate; sluttkode `3aa41334` plasserer det 2 cm foran bakveggen. Kun denne offseten og forklaringen ble endret. Sluttkoden består editor/Linux-pakking, 1278 selvtester, fire High/VHS/1440p-modellvinkler og N01/N02/A02/P01-pilot. Sluttbildene er visuelt kontrollert. Automatiske API-scenarioer er ikke menneskelig gjennomspilling eller lydlytting.

Installert i ~/Games/Signal47/Linux med 40 kontrollerte payloadhash og 51 Saved-filer uendret. Sikkerhetskopi: /home/tom/scratch-bm/Signal47-radioarray/installed-backup-1792792300737671855. Redigerbare leveranser: ~/Desktop/Signal47 radiostasjon og array. Evidens: ~/scratch-bm/Signal47-radioarray. Content-lenker og genererte OpenOrder-endringer er ryddet. Ingen push eller PR.

## Første Caliche-gatebit, 24. oktober 2026

- Caliche-gatebiten er lokalt flettet fra codex/caliche-street og installert i ~/Games/Signal47/Linux. Original suvenirbutikk med butikkrom, sju nye native deler og kontaktjustert bakke rundt eksisterende bensinstasjon og 1420 AM. Steinmaterialer, lokalt lys og faktisk dagslys er rettet etter første visuelle gjennomgang. 212 importkontroller, 84 native kontroller, 1305 selvtester, dør/bevegelse/buzzer, fotopilot og fem korte historieregresjoner PASS. 24 bilder ved 1,62 m i to tidslinjer og dag/skumring/natt er visuelt kontrollert; sekundær kode-/bildegjennomgang uten blokkerende funn. Kjørt sekvensielt med guard, ingen brukerprosess stoppet. 40 payloadfiler hashkontrollert; 51 Saved-filer uendret. Sikkerhetskopi: /home/tom/scratch-bm/Signal47-caliche-street/installed-backup-1792799774134625695. Kilder og bilder: ~/Desktop/Signal47 Caliche gatebit; bevis: ~/scratch-bm/Signal47-caliche-street. Testet C++/config 61120335, native c20beabe, pakke 485f3b19832a. Automatiske scenarioer er ikke menneskelig gjennomspilling, lytting eller fullnatt. Ingen ny ekstern publisering, push eller PR. Midlertidige Content-lenker og OpenOrder-endringer ryddet.
- Blender/Fbx bruker eksplisitte ulike aksekontrakter for butikk og bakke. Ground/glass er NoCollision; behold gamle ganggulv. Uniontrim er atomisk og bevarer fortausmaterialet. Normalstyrke/UV-skala har eksplisitte importparametre; UE-pinnen heter UVs. Midday gjelder bare render fra 05:40, uten å endre Dawn eller historiegater. Se Signal47/Tools/calichestreet/acceptance.json.


## STATION01 og SARO-arkiv, 25. oktober 2026

- Tom prioriterte feltstasjon og interiør foran skyggemannmodellen. 29 originale deler, 87 geometriske LOD-er; kilder i Tools/fieldstation, fieldinstruments og archiveprops. Tre portable Blender-originaler med pakkede teksturer på Desktop/Signal47 feltmodeller 25okt.
- S47FieldModels bruker atomisk validering av native assets og presise forgjengere; 73 patcher, 60 helt skjulte gamle blader og13 eksakte restmesher. Ekte interaksjonsforeldre, skuffer, foto-/gangpunkter, kabelbrudd, papir og KSU beholdes. Skala/lampe leser originale materialtilstander. FBX-akse: UE=(100x,-100y,100z).
- Native F4/1191 og609 lagrede LOD-geometrikontroller PASS. glTF-restimport mistet tynne trekanter og ble avvist; bruk kontrollert FBX-løype. F3/093a4326 består fem fokuserte pakkede scenarioer og18 visuelt vurderte High/Clean-bilder. F1/F2 ble avvist på store gamle glød-/støvkort; F3 retter nære kort/støvkorn uten global lysendring.
- Feltrollens nattfoto er svært mørke. Foto/fremkalling/innsamling og ekte disk-Continue er prøvd via native API. Ingen fysisk Xbox, menneskelig gjennomspilling, lytting, fullnatt eller helruteytelse. Eksisterende RT-minnevarsel beholdes. Se FIELD_MODELS.md og acceptance.json. Fletting/installasjon føres separat.

## SARO-parkering, forplass og avstandsvisning, 2026-10-25 kl. 13:22

- Mat er valgfritt. Alle tre VISITOR-felt virker begge veier; hele direkte kjørerute og telefonoverlevering uten mat PASS. Tre leselige skilt og kontekstmål ved SARO.
- Østfløyen har asfalt/personalparkering; sedan og gangcollider flyttes til x35. Bakt kantvegetasjon 230 tri og gamle hjulrester 144 tri fjernes presist; 10328 dressingtrekanter beholdes. Tools/saroforecourt/prepare_cpu.py må beholde CPU-data på LegacyTrim før cook i andre arbeidskopier.
- Eksisterende flora-krymping beholdes; usynlige instanser utelates.57 hovedstolper mot 82; 65 m spenn med faste avgreninger. Ingen FPS-garanti.
- P1 elleve forløp og P2 målrettet sluttkontroll/1405 selvtester/36 vurderte bilder PASS. P2 avviker bare i rendertrim/audit; øvrige testinnganger er hashkontrollert. Samme P2 installert, 40 payloadhash og 40 uendrede Saved-filer; installert smoke PASS. Se VISITOR_PARKING.md og scratch-bm/Signal47-visitor-fix.
- AGENTS.md og prune-test-builds.py gir gjeldende+to eldre testpakker. To gamle payloadkopier pensjonert, kilder/Saved/bevis bevart. Fysisk Xbox, full 110 og menneskelig fullnatt/lytting er uprøvd.


## Privat Windows-byggpakke, 27. oktober 2026

Tom aksepterte bygging på jobb-PC med Windows. Privat kilde-/innholds-ZIP fra terreng-R2 er levert på egen Drive: https://drive.google.com/file/d/15j_9gE8UQdCMEnGH_thU4_An7YkHRcrY/view?usp=drivesdk . 6 526 897 000 byte, faktisk nedlastbar og eierprivat. Dette er IKKE en ferdig Windows-spillutgave. Windows-kompilering/cook/oppstart/spilling er ikke prøvd. Brukeren må installere Unreal Engine 5.8.4 og MSVC/Windows SDK og kjøre Build-Windows.cmd i utpakket mappe. Kilde og 463 frosne assets er kontrollert; bare leveringskopien har Win64/DX12 SM6-tilpasning. Uferdig MetaHuman er ikke med. Gjeldende Linux-spill og personlige lagringer er uendret. Detaljer i log.md og /home/tom/Documents/ChatGPT/Signal 47/windows-byggpakke/delivery.json.


## Fotoreferanser fra Tom, mottatt 27. oktober 2026

Samlingen ArtSource/References/NewMexico-1980s-2026-10-27 er ferdig mottatt foreløpig: 14 unike originaler fra 15 opplastinger, med README og manifest. Brukes til Sierra, Mesa Diner, Caliche-fasader, gatefunksjon og kjøretøydetaljer. Originaler er lokalt Git-ignorert og skal ikke publiseres eller brukes direkte som spillteksturer. Skill flatskjerm/1994-/©2015-detaljer fra 1986. Bevar tydelige innganger, overdekkede gangveier, praktisk parkering og faktiske materialoverganger.


## Reposamling, 2026-10-27 kl. 14:46 CET

Lokale leveranser og 42-siders designbibel fra `01a17d06-3c66-78f3-b21b-b784af2cc91a` er samlet på `codex/local-deliveries-design-bible-20261027`. Se [samlet indeks](../Docs/LOCAL_DELIVERIES_2026-10-27.md). PDF er byteidentisk; kilder/bildeavhengigheter er med og identiske assets deduplisert. Ny MetaHuman-/atmosfærekode, egne verktøy og runtimebilder inkluderes. NP1/T02/SC01-kvitteringer arkiveres; SC01s dbae3177 er allerede på GitHub, men er ikke integrert i dagens main-baserte bygg. Ingen eldre runtime overskriver nyere levering. Lisensierte råassets, rålogger/private chatter og Saved utelates. Filhash/lenker kontrolleres; ingen ny tung jobb eller spillrelease. Full 110-matrise gjenstår før main-merge av samlet runtime.


## Reposamling publisert, 2026-10-27 kl. 14:48 CET

[PR 172](https://github.com/Tombonator3000/SIGNAL-47/pull/172) er opprettet som utkast og knyttet til samtalen. Kildegren er pushet. Indeks og 42-siders PDF er anonymt nedlastet fra GitHub med HTTP 200; PDF-hash er identisk etter overføring. Python/JSON, 68 designmanifest-hash, 173 tilleggspekere/kopier og inngangslenker består. Full 110-matrise/main-merge og ny spillutgave er ikke utført. Kun rå testlogger og axis_probe-diagnostikk står igjen lokalt ustaget.

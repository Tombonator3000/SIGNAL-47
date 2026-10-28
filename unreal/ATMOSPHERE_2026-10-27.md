# Atmosfære og ytelsesrom, 27. oktober 2026

Punkt 5 etter den lokalt leverte MetaHuman-servitrisen. Samlet R1 er bygget, visuelt vurdert og installert i `~/Games/Signal47/Linux`. Installert standardstarter på Low består ni kontroller, inklusive tre faktiske replikker, pause og diskrestore med én servitrise. Alle 70 personlige Saved-noder med 50 filer er uendret. Ingen ny offentlig spillutgave eller merge inngår. Arbeid og bevis: `~/scratch-bm/Signal47-atmosphere-20261027`.

## Småstein med riktig avgrensning

Caliche_B og Sandstone_A inneholder også steiner opp mot en halv meter. Hele familienes skygger er derfor beholdt. Bare 1 435 forekomster under faktisk 12 cm høyde, målt fra mesh-bounds og instansens Z-skala, er skilt ut i to skyggeløse HISM-batcher. De 12 prototypene gir nå 14 komponenter. Uendrede world-transforms, modellavstander, LOD-er, kollisjon, RT-synlighet og summerte familietall er kontrollert i faktisk pakket spill. Større steiner, planter, bygninger, antenner og klipper beholder skyggene.

Dette er en liten reduksjon av overflødig direkte skyggearbeid. Det er ikke generell avstandsculling av alle planter. Over 1 500 nye romceller ville økt komponent-/CPU-kostnaden og er ikke innført. `-S47LegacySmallShadows` gir gammel policy for faktisk A/B i identisk pakke. Den oppdaterte lesende naturparseren støtter begge skjemaer; alle fem runtime-logger består mot plasseringens opprinnelige instansantall/culls/LOD-er.

## Svak bevegelse uten nye treskygger

Den opprinnelige runtimefloraen hadde allerede fungerende rolig vind. Den er beholdt. Det mer detaljerte Caliche-gresset og gresset i motorveiovergangen var statisk. Bare Grass_A/B får nå en egen kopi av prosjektets foliagemateriale, med samme farger, teksturer og belysning. Busker/trær bruker fortsatt det opprinnelige materialet.

En enkel WPO bruker den eksisterende MPC-klokken og vindstyrken. Røttene står fast, toppforskyvningen er begrenset til høyst 2,2 cm og bevegelsen tones ut mellom 20 og 70 m. Det krever ingen ny tick-/instansoppdatering. [Tidsbilde ved t=8](ArtSource/Atmosphere/Preview/caliche-grass-t8.png) og [t=10](ArtSource/Atmosphere/Preview/caliche-grass-t10.png) er faktiske HUD-frie rendererbilder med samme kamera. Faktisk WindTime/WindAmp er avlest fra MPC og stemmer med vinddriveren. Dette er ikke en påstand om menneskelig bevegelsesvurdering eller at hvert blad er målt i piksler.

## Lyd som følger kveld og natt

Den gamle nattmodellen antok oppstart kl. 21. Ved 17:20 ble kvelden dermed tolket som rundt 20 timer inn i en nedkjølt natt, omtrent minus 8 grader, før temperaturen hoppet til 16 grader kl. 21. Det ga feil eller manglende nattinsekter i den utvidede kvelden.

Kjølingen er nå kontinuerlig gjennom kvelden. Nattinsekter fases mykt inn 18:00 til 19:00; ugle/poorwill følger nattfasen. Den etablerte kurven 21:00 gjennom daggry er bevart. Sju regresjonskontroller dekker ettermiddag, skumring, kontinuitet ved 21:00, natt/daggry og negative/gjenopprettede døgnklokker. Hush, Breath, Still, Back, innendørs lavpass/demping, vindlag, hund/tog/lastebil, stemmer og radio er bevart. Ingen nye lydassets eller parallelle ambience-lag er lagt til. Dette er kilde-/runtimekontroll, ikke menneskelig lytting.

## FSSS ble prøvd og forblir av

En svak eksperimentvariant ble sammenlignet ved samme SARO-inngang kl. 03:00, High 1600 × 900. Tetthet, lys, eksponering og himmel var uendret. Varianten brukte scene scale 0,07, power 1,5, spread 0,3 og blur 0,15. Native GPU-profil bekrefter faktisk FSSS-mipkjøring, ikke bare en satt bool. [Vanlig bilde](ArtSource/Atmosphere/Preview/saro-fsss-off.png) og [eksperiment](ArtSource/Atmosphere/Preview/saro-fsss-experiment.png) ga ingen tydelig visuell forbedring i dette utsnittet.

FSSS kostet omkring 0,34 ms ekstra GPU-tid i denne prøven. Mipkjeden alene brukte 0,220 ms i det ene profilerte bildet. Standard er derfor fortsatt av. Den begrensede `-S47FogScattering`-diagnostikken er bevart for senere sammenligning, bare ved SARO på High/Ultra i mørke/skumring. Low holder den av. Ingen uprøvd effekt eller Lumen Lite-preset er aktivert.

## Faktisk måling og aksept

Alle målinger bruker samme R1, RTX 3080, Vulkan og 1600 × 900 High. Native GPU-tid er beregnet fra samme varme t≈8 til t≈18-vindu, ikke fra oppstart eller siste skjermbildehakk.

| Utsnitt/variant | GPU, ms | Tolkning |
| --- | ---: | --- |
| Caliche, gamle småsteinskygger, statisk detaljgress | 12,100 | A/B-referanse |
| Caliche, ny småsteinspolicy, statisk detaljgress | 12,035 | Forskjellen 0,065 ms er beskjeden og innen vanlig variasjon |
| Caliche, ny policy og gressvind | 11,988 | Egen måling; inkluderer også selvtestoppsett, ingen presis isolert vindgevinst hevdes |
| SARO, FSSS av | 9,646 | Beholdt standard |
| SARO, svak FSSS | 9,986 | Ingen tydelig bildegevinst for ekstrakostnaden |

Ett GPU-profilerbilde viser 490 til 481 draw-kall under ShadowDepths, og non-Nanite VSM 0,955 til 0,850 ms, i småsteinssammenligningen. Dette er ett bilde, ikke stabil helrutegevinst. Gressmålingens profilerbilde hadde et ShadowDepths-utslag på 4,979 ms; derfor brukes det varme GPU-vinduet, og ingen sikker FPS-gevinst hevdes. Ingen påstand om «2x raskere Lumen» følger av dette arbeidet.

Sluttbygg for editor og samlet cook/pakking består. Pakket selvtest har 1 593 kontroller, inklusive de sju nye lydkontrollene. Fem avgrensede rendererprosesser består; sju originalbilder er faktisk åpnet og vurdert. Naturens schema-, sum-, skygge-, LOD- og cull-kontrakt består i alle fem. Parseren er også kontrollert mot gammel faktisk R2 og åtte korrupte logger; syntetiske parserprøver er ikke merket som runtimebevis.

Kilde-/assetmanifestet binder 12 982 filer. Alle tidligere Content-filer, inkludert MetaHuman, godkjent daghimmel og natt/stjerner, er byteidentiske; bare det nye originale gressmaterialet er lagt til. Selve spillhistorien og lagringsformatet er uendret. Ingen full 110-bilders merge-matrise, helnatt, menneskelig input/lytting eller helruteytelse er attestert her.

## Kilder og levering

[Materialkilden og diagnostikken](Signal47/Tools/atmosphere/README.md) og den eksisterende naturverifikatoren ligger i repoet. Prosjektbiblioteket ble søkt først: scenario-labs sin Unreal-lysreferanse ved `f0adb5b5` var en arbeidsflytreferanse med selvoppgitt UNVERIFIED-verktøystatus. Ingen skript eller plugin derfra er gjenbrukt. Implementasjonen bygger på eksisterende prosjektkode og installert UE 5.8.4, særlig FogSeparateComposition, ExponentialHeightFogComponent og LocalVertexFactory.

MetaHuman R3 er bevart i denne kandidaten; lokale råassets, kilder, innloggings-/Creator-staging og separat SC01 røres ikke. Installasjonen bruker den eksisterende verifiserte reflink-/rsync-løypa med Saved unntatt. `installed-verification.json` dokumenterer identisk payload, bevarte Saved og 212 tale-/ansiktsmålepunkter i installert Low, med kjevekurvemaks 0,576317 og 23,644123 graders kjevebevegelse. Bildet etter lasting er faktisk åpnet og vurdert. Canonical prune er anvendt på to eldre MetaHuman-mellomkandidater; nå beholdes atmosfære-R1, to eldre fungerende R3-kopier og særskilt beskyttet SC01-R4. Kilder, assets, lagringer og bevis er bevart. Oppfølgingen er slettet i appen etter bekreftet levering; samtalen er beholdt. Ingen ny GitHub-spillutgave publiseres automatisk.

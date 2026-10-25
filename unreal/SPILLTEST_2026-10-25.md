# Spilltest av SIGNAL / 47, 25. oktober 2026

Status: ferdig testrapport. Spillet kan ikke godkjennes som ferdig gjennomførbart på den testede maskinen: finalen krasjet i tre separate prøver, også på Low. I tillegg krasjet én bortføringsprøve. Rotårsak er uavklart.

Den brede serien besto 108 av 110 scenarioer. Tilleggsserien besto 12 av 13; den separate Low-finalen feilet. 1 405 interne selvtester besto. Dette er automatiserte runtimeprøver og bilde-/kodegjennomgang, med begrensningene nedenfor, ikke en ubrutt menneskelig gjennomspilling.

## Oppdrag og testgrunnlag

Tom ba om å prøve hele spillet som testspiller, finne feil og mangler, rapportere hva som fungerer og foreslå forbedringer. Denne rapporten dekker den installerte Unreal-utgaven etter PR157. Den nye historie- og stealthplanen er ikke implementert innhold som kan godkjennes gjennom denne testen.

Native vindusstyring er ikke tilgjengelig gjennom verktøyene i denne økten. Testen bruker derfor spillets eksisterende native kontrollruter, kapittelfiksturer, syntetiske Slate-taster, lagringsprøver, runtimebilder og lesende kodegjennomgang. Dette er ikke en blind menneskelig gjennomspilling eller én ubrutt kveld til slutt. Opplevd frykt, lydmiks, forståelse uten forkunnskap og menneskelig spilletid er ikke målt.

- Spillkilde: `1e89ae00d07b1e891489cd44eeae8412094d9322`, `unreal-port-core`.
- Installert spill: `/home/tom/Games/Signal47/Linux`.
- Maskin: i7-12700KF, RTX 3080 10 GiB, NVIDIA 595.99.02, Linux/Vulkan.
- Bred test: 1600 x 900, egen testprofil, rendring uten synlig spillvindu. Hvert scenario starter en ny prosess.
- Tillegg: ny tidslinje, kapitteloverganger, full finale, åpningsvarianter og den nyere anomaly-piloten. Eksakte argumenter og inngrep lagres per kjøring.
- Ingen spillkode, modeller, balanse, personlig lagring eller installerte filer er endret av rapportarbeidet.

Alle tunge spillkjøringer ble kjørt sekvensielt med `unreal_guard`. Ingen prosess som tilhører Tom ble stoppet. Før/etter-kontrollen er dokumentert i sluttresultatet.

Prosjektbiblioteket og START_HER ble lest. Behovssøk etter Unreal/spilltesting og ett bredere søk ga ingen verifisert, kompatibel gjennomspillingsløsning. Eksisterende prosjektverktøy brukes. Ingen nye plugins, bibliotekskode eller assets er hentet. The Dark Mod ble kun kontrollert som en kildebasert stealth-referanse; annen motor og UNVERIFIED teststatus gjør den uegnet som komponent her.

## Hvordan resultatene skal leses

| Grunnlag | Hva det viser | Hva det ikke viser |
|---|---|---|
| Kjørt kontrollrute | Spillet kjører de registrerte handlingene, flyttingene og kontrollene | At en ny spiller oppdager handlingene selv |
| Ordinær kjøre-/gangkontroller | Faktisk bevegelse, kollisjon og områdeskifte langs testens rute | Fri menneskelig navigasjon eller alle mulige parkeringsforsøk |
| Kapittelfikstur | Aktive systemer, UI, bilder og overganger fra en forberedt tilstand | Hele veien frem til tilstanden; flere drivere setter bevisflagg direkte |
| Syntetiske Slate-taster | Den registrerte UI-tastesekvensen behandles av spillet | Fysisk tastatur/Xbox eller fri input gjennom hele natten |
| Bildegjennomgang | Synlig tilstand i de konkrete bildene | Lyd, bevegelseskomfort, flimring over tid eller samlet stabilitet |
| Kildefunn | En konkret regel eller tekst finnes i testet kode | At spilleren faktisk har møtt konsekvensen under kjøringen |

## Kjørte kontroller

| Kontroll | Resultat | Faktisk dekning |
|---|---|---|
| Hele eksisterende `screenshots.sh` | 108/110 PASS | 110 separate prosesser, 108 PNG-er. Bortføring og sluttittelprøve feilet med GPU-krasj. Ingen konkurrerende tung Unreal-jobb. |
| Interne selvtester | 1 405 PASS | Regler og tilstandskontroller. Godkjenner ikke grafikk, lydmiks eller oppdagbarhet. |
| UI og dokumenter | Vurdert | Alle seks Settings-faner, notatbok, terminal, dokumenter og brett. Små layoutfunn og synlig motorvarsel; nødvendig tekst var leselig i de vurderte utsnittene. |
| Ny tidslinje, kapittel 1 til 2 | PASS | FILE via syntetiske Tab/Tab/Enter. Reelt nytt kapittel 2-sjekkpunkt, v3/tidslinje 2. Oppgaven før FILE er forberedt av driveren. |
| Ny tidslinje, kapittel 3 til 6 | PASS | Faktiske overgangscallbacker i én prosess, med to ekte felteksponeringer. Foto-/fremkallings-/dialogflagg og forflytninger er delvis injisert. |
| Frisk Continue etter kjeden | PASS | Kapittel 5 fra siste sikre sjekkpunkt. Tre fotoassets med identiske før/etter-bytes og bevarte diskreferanser/tider/targets. Lastet fototilstand i minnet er ikke dumpet separat. |
| Ny High/Clean-finale til meny | FAIL | Andre finalekrasj omtrent åtte sekunder inn i uttrekket; ingen sluttittel eller meny. |
| Low/Clean-finale | FAIL | Samme finale krasjer omtrent 7,1 sekunder inn i uttrekket. Ingen sluttittel, rulletekst, sluttfoto eller meny. Low er ingen bekreftet omvei. |
| Direkte kveldskjøring uten mat | PASS | 2622,7 m med ordinær kjøre-/gangkontroller, SARO-ankomst, telefon, disklast og tre papirer én gang. 201,2 s styrt kontroll, ikke menneskelig spilletid. |
| Burger, sandwich og vann | PASS | Tre separate valgfrie åpningsruter til SARO og Dales overlevering. Mat-/vannresultat, disklast og fravær av utilsiktet kapittel 5-overgang kontrolleres. |
| Ny anomaly-pilot High/VHS og Low/Clean | PASS | Begge kontrollruter består, med foto, lagringsprøver, figurtilstand og printer-/studiohendelser. Bildegjennomgangen er delvis: den svake andre formen ved vaskerivinduet er ikke sikkert identifisert. |
| Manglende anomaly-foto | PASS | Den slettede filen i egen testprofil blir ikke godkjent som gyldig bildebevis. Veien videre med utilstrekkelig bevis er tilgjengelig. |

Råbevis beholdes i [testmappen](/home/tom/scratch-bm/Signal47-playtest-20261025), med [samlet matrise/logg-/bildeindeks](/home/tom/scratch-bm/Signal47-playtest-20261025/evidence-summary.json), [tilleggsresultater](/home/tom/scratch-bm/Signal47-playtest-20261025/supplement-results.json) og [kontaktark](/home/tom/scratch-bm/Signal47-playtest-20261025/contacts/index.json). OK-etiketter på kontaktarkene er scriptresultat, ikke en godkjenning av hver detalj i bildet.

Anomaly-verktøyet krever en bevart kandidat utenfor installasjonsmappen. Det ble kjørt mot kandidaten med de samme 40 payloadfilene som den installerte utgaven, kontrollert før og etter. [Pakkebinding](/home/tom/scratch-bm/Signal47-playtest-20261025/anomaly-candidate-binding.json). Ingen ny spillpakke ble laget.

Det brede `screenshots.sh`-utvalget bruker i stor grad gammel tidslinje og uavhengige utviklerhopp. Den nye kveldstidslinjen må prøves særskilt. Åpningsdriveren avslutter programmet etter Dales overlevering. Kapittel 1-driverens `report`-variant gjør bare én faktisk eksponering før den forbereder rapporttilstanden; andre varianter kan ta to. Kapittel 3 til 5-driverne flytter spilleren og fyller også enkelte bevis-/fremkallingsflagg. Ingen av disse er et bevis på ubrutt spilling.

Et kjent dekningshull er normal kapittel 2 til 3-overgang: prosjektet har ikke en eksisterende runtime-driver for hele arkivoppgaven og den avsluttende telefonsamtalen. At et hopp til kapittel 3 virker, godkjenner ikke dette mellomleddet. Ingen ny driver skrives bare for å skjule hullet.

## Det som fungerer

De interne 1 405 selvtestene besto. De kontrollerte konsoll-, dokument-, notatbok- og bevisbrettbildene har leselig påkrevd innhold ved 1600 x 900. Konsollen viser aktive frekvens-/gain-/båndbredde-/azimuthkontroller. Dales håndskrevne logg har lesbart transkript. Notatbokas hovedoppgaver og bevisbrettets tråder kan leses i de vurderte utsnittene.

Alle fire åpningsrutene fullfører: direkte uten diner, burger med mat tatt med, sandwich spist på diner og vann uten matkjøp. De går videre til SARO og Dales telefonoverlevering. Ingen observert matport eller parkeringslås oppstår i disse rutene. Dette beviser de prøvde rutene, ikke alle mulige frie kjøremanøvre.

Kildegjennomgangen viser en sammenhengende implementert historie: valgfri kveldsmat og SARO-overlevering, RX/residual og signalhendelse, to eksponeringer og B-12-kontroll, arkivets endrede rapport, STATION 01, Nora i Rom 6, dinerens kart og den siste kjøreturen. Hvert ledd har som regel et konkret steds- og handlingsmål. Dette er kildegrunnlag, ikke en påstand om at hele kjeden er spilt uten hopp.

Det finnes gode gjenopprettingsregler: feil mørkeromsbehandling beholder negativet, brettet viser manglende bevis og progressive hint, dialog har tekststøtte, og lagringen bruker midlertidig fil og roterende autosaves. Det ligger et tilsiktet trygt sjekkpunkt før den siste turen.

De nye bygningsmodellene lastes faktisk. Den ferske oppstartsloggen registrerer seks bygningsfamilier, 28 native deler, 32 sammensatte patcher og 60 erstattede gamle tegneflater, i tillegg til 29 feltdeler og servicebilen. Modellerte interiører innebærer fortsatt ikke at alle bygg er åpne historieområder.

Den nyere anomaly-ruten består både på High/VHS og Low/Clean. Den ekstra svarte formen ved pappfiguren og ON AIR-skiltet kan sees i begge profiler. Vaskeriets sammenligningsbilde viser lampe, plate og ulike vinkler, men den oppgitte andre formen i vinduet er vanskelig å identifisere sikkert. Denne detaljen trenger en vanlig spillertest; loggført korrekt foto er ikke tilstrekkelig visuell godkjenning. [Seks vurderte bilder](/home/tom/scratch-bm/Signal47-playtest-20261025/visual-anomaly-review.json). Respons på tilnærming og figurtilstand kontrolleres av runtime-ruten; stillbildene alene beviser ikke bevegelse eller lyd.

Noen tilsynelatende feil skyldes testopptaket og er ikke ført som spillfeil. Svarte CRT-er i `shift`-bildet er korrekt strømtilstand for dette hoppet. `direct-18-phone.png` tas samme frame som telefonen åpnes, før første undertekst starter 0,08 sekunder inn i talen. Fravær av tekst i akkurat det bildet er derfor ikke bevis på tom spillerreplikk. Captions som overlapper i tvungne eventfiksturer er heller ikke automatisk et bevis på vanlig hendelsesrekkefølge.

## Feil og konkrete mangler

P1 betyr stor risiko for fremdrift eller forståelse. P2 betyr relevant funksjons-, presentasjons- eller tilgjengelighetsproblem. P3 betyr mindre tekst-/layoutfeil. Prioritet er en vurdering; observerte forhold og kildefunn merkes hver for seg.

### QA00, P1: Vulkan-krasj i bortføring og finale

**Faktisk runtimefeil.** Matrix-scenarioet `abduct` krasjet i én registrert kjøring etter at spilleren kom tilbake og en skikkelse sto over spilleren. Loggen registrerer warning, light, taken, back og one of them over you, deretter `vkQueueSubmit` med `VkResult=-4 / VK_ERROR_DEVICE_LOST` ved 13:08:42 UTC. Forventet sluttskjermbilde og smoke-OK ble ikke produsert. [Krasjlogg](/home/tom/scratch-bm/Signal47-playtest-20261025/matrix/abduct.log).

Testen bruker den installerte PR157-binæren, Vulkan, High og 1600 x 900, med egen profil og ingen annen tung Unreal-jobb. Sceneargumentene er `-S47Jump=chapter1 -S47Face=at:-20;30;3.14;0 -S47Abduct=4 -S47Seconds=24 -S47SmokeTest`. Anførselstegn må brukes rundt argumentet med semikolon ved manuell shellkjøring.

**En annen faktisk runtimefeil rammer finalen.** `chapter6title` starter THE EVENT, fullfører de 47 sekundene og begynner uttrekket ved 05:29:47. Omtrent 7,3 sekunder senere, 13:29:24 UTC, kommer samme Vulkan-feilkategori. Sluttittel og skjermbilde blir ikke produsert. Sceneargumenter: `-S47Jump=chapter6:line -S47Auto=20 -S47Seconds=99 -S47SmokeTest`. [Finalekrasj](/home/tom/scratch-bm/Signal47-playtest-20261025/matrix/chapter6title.log). Samme feilkategori beviser ikke samme rotårsak.

**Finalefeilen gjentas på ny tidslinje og Low.** High/Clean-prøven som skulle dekke hele avslutningen, krasjer ved 13:36:34 UTC, omtrent 7,6 sekunder etter uttrekkets start. Den gamle prøven bruker Carpenter-filter. Den separate Low/Clean-prøven krasjer ved 13:57:12 UTC, omtrent 7,1 sekunder etter uttrekkets start. Feilen er dermed ikke avgrenset til bare gammel tidslinje, ett bildefilter eller High. Ingen av de tre finaleforløpene når sluttittel, rulletekst, sluttfoto og meny. [Ny High-logg](/home/tom/scratch-bm/Signal47-playtest-20261025/supplement/new_finale_through_menu/run.log), [Low-logg](/home/tom/scratch-bm/Signal47-playtest-20261025/low-finale/run.log), [Low-resultat med eksakte argumenter](/home/tom/scratch-bm/Signal47-playtest-20261025/low-finale.json).

De avvikende naboprøvene `abductframes` og `abductover` besto i egne prosesser. To senere retester av samme `abduct`-parameteroppsett besto også. Returposisjon, yaw og tidsbortfall randomiseres; dette er ikke identisk scene/RNG. Disse PASS-resultatene opphever ikke krasjet og gir ingen målt krasjprosent for vanlig spilling. [Lesende krasjaudit](/home/tom/scratch-bm/Signal47-playtest-20261025/crash-review.md) skiller de fire unike krasjprosessene fra kopier av de samme motorloggene.

Low-loggens aktive GPU-breadcrumbs mangler flere pass som finnes i High, blant annet Capture/Lumen/VolumetricCloud/GenCubeMips. En forklaring begrenset til bare disse High-passene dekker derfor ikke alle tre finalekrasjene. Det beviser heller ikke at ett bestemt gjenværende pass er årsaken.

**Forslag:** Prioriter dette før nye større spillsekvenser. Reproduser samme forløp på samme pakke og undersøk feilen med begrensede, kontrollerte A/B-prøver. Flere GPU-pass er aktive i breadcrumb-listen; hverken minnevarselet, himmel, modell, flora, driver eller skygger er bevist som rotårsak. En vellykket kortprøve kan ikke lukke feilen. Ny godkjenning må omfatte hele THE EVENT, uttrekket og avslutningen til meny.

### QA01, P2: Motoradvarsel ligger over spill og dokumenter

**Observert.** `room.png`, `log.png`, `console.png` og flere notatbokbilder viser en rød raytracing-advarsel øverst i spillerbildet. På bevisbrettet går den over HOW-instruksjonen, og på dokumenter over topptekst. Eksempel: [kontrollrom](/home/tom/scratch-bm/Signal47-playtest-20261025/matrix/room.png).

Teksten viser omtrent 84,85 MiB av et 400 MiB-budsjett ved en 20 prosent-terskel. Dette er ikke bevis på at kortets 10 GiB er fullt eller på et krasj. Varslet er likevel uønsket spiller-UI.

**Forslag:** Undersøk den konkrete raytracing-residensen og sørg for at utviklerdiagnostikk ikke ligger over det ferdige spillerbildet. Et rent skjermbilde er ikke i seg selv en løsning på et eventuelt ressursproblem.

### QA02, P2: Material- og skyggevarsler krever egen grafikkretting

**Observert logg og kontrollert kode.** Oppstart logger 70 material-usage-varsler, 58 for InstancedStaticMeshes og 12 for Nanite, samt `Non-Nanite Marking Job Queue overflow`. Seks ISM-varsler gjelder native antenners M_Array-materialer. Tre aktive ISM-komponenter bruker disse materialene og har 27 instanser hver, uten senere materialoverstyring. Unreal bytter section-material til Default når denne usage-kontrollen feiler. Eksisterende native audit kontrollerer navn, bindinger og LOD-er, men ikke dette flagget. [Fersk logg](/home/tom/scratch-bm/Signal47-playtest-20261025/matrix/selftest.log), [presis kildeaudit](/home/tom/scratch-bm/Signal47-playtest-20261025/material-review.md).

De konkrete påvirkede antenneflatene er ikke avlest visuelt i denne auditten. Alle andre varsler kan heller ikke automatisk tolkes som synlig fallback: noen eksportmaterialer overstyres eller skjules senere. Eksporten har dessuten enkeltmateriale/flersection-mapping som må avklares. Livefloraen bruker egne ISM-merkede mastere og er ikke påvist berørt av dette. VSM-varselet er registrert ved oppstart og viser ikke målt FPS-tap i en normal rute.

**Forslag:** Kontroller materialene på faktisk aktive ISM/HISM-seksjoner i ferdig cook. Rett import-/usage-/materialmapping på den berørte familien, og retest de samme dag-/nattutsnittene. Profilering av skyggejobber bør styre videre optimalisering.

### QA03, P2: Quit to title varsler ikke om uspart fremdrift

**Kildefunn, ikke ny observert runtimefeil.** Pausemenyens QUIT TO TITLE kaller `AutosaveNow()` og går videre til tittelen. `AutosaveNow()` returnerer stille når lagring er blokkert. Den siste kjøreturen blokkerer alltid lagring, og kapittel 3 har midlertidige sperrer. Spilleren kan dermed forlate en del av forløpet uten en forklaring på hvor Continue vil starte.

Kilder: [S47Flow.cpp:847](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/S47Flow.cpp:847), [S47Flow.cpp:1069](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/S47Flow.cpp:1069), [S47Chapter6.h:30](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Public/S47Chapter6.h:30).

**Forslag:** Vis siste sikre sjekkpunkt og forklar hva som må gjentas før utgang. Behold sperren der en mellomtilstand faktisk ikke kan lagres trygt. Kontroller separat at feil ved filskriving gir en begripelig utgangssituasjon.

### QA04, P2: Obligatoriske kameraoppgaver hardkoder tastene

**Kildefunn.** De to hovedmålene for eksponering viser C og Space selv når kamera-/utløsertastene er ombundet eller Xbox brukes. Xbox har X/RT. Dette kan gjøre riktig handling vanskeligere å finne.

Kilder: [S47Chapter1.cpp:491](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/S47Chapter1.cpp:491), [S47Chapter1.cpp:512](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/S47Chapter1.cpp:512). Dynamiske kontrollnavn finnes allerede i `S47Keys::Said`.

**Forslag:** Bruk gjeldende bindingsnavn i mål og hjelp. Prøv begge kameraoppgavene med ombundet tastatur og fysisk Xbox.

### QA05, P2: Tilgjengelighetsvalgene dekker ikke hele opplevelsen

**Kildefunn.** Large text gjør ikke bevisbrettets 11,5 til 12,5 px-felt større. Brettzoom finnes, og påkrevd tekst kunne leses i det vurderte 1600 x 900-bildet; dette er et tilgjengelighetsgap, ikke en observert puslespillblokkering. Vanlig gange har dessuten bob/roll/shake uten eget avvalg, mens komfortvalgene særlig dekker sluttkamera og filmforstyrrelser. Kvalme er ikke målt.

Kilder: [SS47Board.cpp:554](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/SS47Board.cpp:554), [SS47Settings.cpp:369](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/SS47Settings.cpp:369), [S47Walker.cpp:272](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/S47Walker.cpp:272).

**Forslag:** La tekststørrelse også omfatte obligatoriske bevisoppgaver. Gi et tydelig valg for hodebevegelse og støt som kan prøves under vanlig gange og kjøring.

### QA09, P2: Kartoppgaven kan godkjennes uten kartkontrollen

**Kildefunn, ikke runtime-observert i denne testen.** P13-panelet tilbyr alle fire svar før kartoverlegget er justert. `Answer(bm8)` godkjenner riktig svar uten å kontrollere `Aligned()`, som bare brukes ved feil svar. Notatet hevder deretter at kartoverlegget var lagt på STATION 01. En riktig gjetning kan dermed bli ført som utført undersøkelse.

Kilder: [SS47MapOverlay.cpp:224](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/SS47MapOverlay.cpp:224), [S47Chapter5.cpp:785](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/S47Chapter5.cpp:785).

**Forslag:** Hvis den fysiske kartkontrollen er nødvendig bevis, krev riktig justering før konklusjonen registreres. Hvis riktig svar skal kunne gjettes, må notatet ikke påstå en kontroll som ikke er utført. Behold en tydelig vei videre ved feil svar.

### QA10, P2: Feltfotoene har svært svakt synlig motiv

**Observert i faktisk lagrede foto.** FRAME 03 og 04 fra kapittel 3 er dekodet direkte fra lagringsbutikkens data-URL-filer. Bildene er uendrede originalbytes etter dekoding. Oppgaveregistreringen og metadata har nødvendige targets, men særlig kabelbruddet er nesten svart i bildepikslene. Papirrammen og bildeteksten er tydelige; detaljene man skal undersøke er svært vanskelige å se.

Bevis: [fastpunktfoto](/home/tom/scratch-bm/Signal47-playtest-20261025/decoded-field-photos/pnfi6gx3v0j.jpg), [kabelbrudd](/home/tom/scratch-bm/Signal47-playtest-20261025/decoded-field-photos/p873wjv2myl.jpg), [kilde-/bildehash](/home/tom/scratch-bm/Signal47-playtest-20261025/decoded-field-photos/index.json).

Dette gjelder den automatiserte natt-ruten uten ekstra lyktbelysning. En fri spiller kan velge annen belysning/posisjon; dette er ikke bevist som en uunngåelig progresjonsblokkering. Det viser likevel at vellykket eksponering og lagring ikke garanterer leselig bildebevis.

**Forslag:** Prøv oppgavene med vanlig spillerveiledning og lykt. Gi tydelig beskjed ved utilstrekkelig belysning, og sørg for at nødvendig merke/kabel kan undersøkes i fotoet gjennom lokal lyssetting eller en begrunnet eksponeringskontroll. Global lysning av hele natten er ikke nødvendig som første tiltak.


### QA06, P3: Små tekstklipp og irrelevant kontrollhjelp

**Observert i runtimebilder.** Notatbokas grønne Display reference-label klippes i funnlista. Metadatalinjen på det skrevne dokumentkortet klippes i høyre kant. Desktopkontrollsiden bruker høyre halvdel på Phone and tablet, mens Xbox-hjelpen ligger i Settings.

Bevis: [funn](/home/tom/scratch-bm/Signal47-playtest-20261025/matrix/notebook-findings.png), [papirer](/home/tom/scratch-bm/Signal47-playtest-20261025/matrix/notebook-papers.png), [kontroller](/home/tom/scratch-bm/Signal47-playtest-20261025/matrix/notebook-controls.png).

Dette hindret ikke lesing av hovedoppgavene. Loggforhåndsvisningen viser ikke hele signaturen, men READ IT åpner hele dokumentet; det er ikke tapt innhold.

**Forslag:** La lange stempler og metadata bryte linje eller få nok bredde. Vis relevante kontrollsider etter plattform og aktiv input, med eksplisitt valg til de andre.

Dinerens hovedmål vises dessuten i fem svært smale linjer i [dinerbildet](/home/tom/scratch-bm/Signal47-playtest-20261025/matrix/diner.png). Hele instruksjonen er til stede; dette er unødvendig smal tekstbryting, ikke mistet oppgavetekst. En stabil, brukbar minimumsbredde bør prøves i HUD-en.

### QA07, P3: Gamle meldinger sier at neste del ikke kan spilles

**Kildefunn.** Complete-tilstandene i kapittel 3, 4 og 5 bruker fortsatt NEXT AREA/PART NOT YET PLAYABLE, selv om neste kapittel er registrert og overgangen følger. Hvor synlig et slikt blink blir, er ikke målt.

Kilder: [S47Chapter3.cpp:548](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/S47Chapter3.cpp:548), [S47Chapter4.cpp:473](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/S47Chapter4.cpp:473), [S47Chapter5.cpp:332](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/S47Chapter5.cpp:332).

**Forslag:** Erstatt med riktig overgangsmål. Spilleren skal ikke få beskjed om at bygget slutter når neste del faktisk begynner.

### QA08, P3: Krediteringen oppgir gammel spillmotor

**Kildefunn.** Unreal-rulleteksten legger fortsatt inn Built with / three.js (MIT). Historiske tredjepartskrediteringer kan beholdes der de gjelder, men denne motorangivelsen beskriver nettversjonen.

Kilde: [SS47Ending.cpp:87](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/SS47Ending.cpp:87).

**Forslag:** Oppdater motorangivelsen til den leverte Unreal-utgaven og kontroller øvrig rulletekst mot gjeldende krediteringsgrunnlag.

## Forbedringer i spillerreisen

Disse punktene er kildebaserte vurderinger av struktur og motivasjon. De er ikke målte reaksjoner fra en førstegangsspiller.

1. **Gi siste tur et nødvendig og troverdig formål.** Nora advarer mot å stoppe på linjen, og sjåføren beskriver motorstopp ved bolten. Likevel er hovedmålet å kjøre dit. Knytt turen til en konkret kontroll eller varsling som krever reisen, og gi hovedpersonen en rimelig plan. Den faste slutten kan beholdes. Se [S47Chapter4.cpp:1186](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/S47Chapter4.cpp:1186), [S47Chapter5.cpp:716](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/S47Chapter5.cpp:716).
2. **Bruk bevisbrettet til slutninger som betyr noe.** Kapittel 1 alene har 13 oppgavetrinn, og senere kapitler legger flere bekreftelser etter fysisk kontroll, foto og samtale. Behold valg som prøver forståelsen. Slå sammen rene kvitteringer og la et funn oftere åpne en fysisk handling, adgang eller et nytt spørsmål direkte.
3. **Gjør skikkelsenes regler konsekvente.** Glimpses forsvinner ved direkte blikk, nærhet, zoom eller lykt. SaroDark tåler vanlig blikk, men reagerer på de andre. Bestem hvilke som er glimt, avtrykk og reaktive aktører. Gi hver kategori en stabil regel og synlig forskjell. En aktiv undersøkelse bør gi forståelig respons.
4. **Gi filmventingen nyttig innhold.** Den nyere piloten har allerede ekstra former på film, en figur som flytter/vender seg usett, og printerlyd som reagerer på tilnærming. Den er ikke bare en plan. Sammenligning krever fremkalling utenfor pausede paneler. Gi en konkret valgfri undersøkelse under ventingen og tydelig fremkallingsstatus.
5. **Kurater skrekktopper mot hele reisen.** Prologen har allerede signal, fremtidslyd, reléutfall, lysfall og antennehendelse. Senere topper bør gi rom for Nora, brevet, kartarbeidet og etterarbeidet. Flere hendelser er ikke automatisk bedre pacing. Mål hvor spilleren faktisk går, venter og åpner paneler før nye topper legges til.
6. **Optimaliser etter målt kostnad.** Reduksjonen i stolper og helt usynlige planteinstanser er allerede levert. Videre arbeid bør styres av GPU-/CPU-/skygge-/streamingprofiler under transport og områdeskifter. Antall objekter alene er ikke en FPS-måling.
7. **Samordne personmodellene med resten av presentasjonen, P2 produksjonsmangel.** De vurderte Rom 6- og dinerbildene viser enkle 3D-personer uten ansiktstrekk, mens dialogen bruker detaljerte portretter. Det gir en synlig forskjell mot de nye byggene og rekvisittene. Interaksjon og dialog er tilgjengelig; dette er en produksjons-/stilvurdering, ikke en fastlåsing. De opprinnelige figurene er bygget av primitive former og beholdt ved miljøoppgraderingen, slik [S47MesaDiner.cpp:62](/home/tom/SIGNAL-47/unreal/Signal47/Source/Signal47/Private/S47MesaDiner.cpp:62) eksplisitt krever. Ingen ny regresjon er påvist. Se [Rom 6](/home/tom/scratch-bm/Signal47-playtest-20261025/matrix/chapter4.png), [dialog](/home/tom/scratch-bm/Signal47-playtest-20261025/matrix/room6talk.png) og [diner](/home/tom/scratch-bm/Signal47-playtest-20261025/matrix/diner.png).
8. **Tilpass parkeringsskiltenes størrelse til stedet.** Skiltene gjør besøksplassene tydelige, men de tre store flatene dominerer forplassen i [ankomstbildet](/home/tom/scratch-bm/Signal47-playtest-20261025/supplement/opening_direct/shots/direct-parking.png). Mindre vanlige parkeringsskilt og tydelig merking på asfalten kan bevare veiledningen og gi en mer naturlig arbeidsplass. Dette er en visuell vurdering; selve parkeringen besto.

Den komplette stealth-loopen, SARO-beleiringen og den nye invasjonskjeden i [HORROR_STORY_PLAN.md](/home/tom/SIGNAL-47/unreal/HORROR_STORY_PLAN.md) er planlagte produksjonsetapper. Fraværet av disse er ikke en regresjon i det installerte bygget. Dagens spill må vurderes som den etterforskningsreisen som faktisk er implementert.

## Det som fortsatt trenger en menneskelig test

- Ny sak til slutt uten utviklerhopp, med faktisk kapittel 2 til 3-samtale og riktige klokker hele veien.
- Om spilleren finner mål, parkering, mørkeromsprosessen, arkivets slutninger og Rom 6 uten kjennskap til testskriptene.
- Lytting med hodetelefoner: tale mot radio/ambience, lydkildenes retning, stille partier, avbrudd og viktige signaler.
- Frykt, forventning, variasjon og hvordan spilleren forstår skillet mellom forklarlige observasjoner og det uforklarte.
- Fysisk Xbox, ombundne kontroller, mindre skjerm og redusert syn/komfortvalg under alle obligatoriske oppgaver.
- Samlet spilletid, andel transport/paneler/venting og ytelse gjennom hele ruten. Automatiseringens sekunder skal ikke omregnes til menneskelig spilletid.

## Sluttresultat og neste prioritet

**Spillet er ikke godkjent som gjennomførbart til slutt på denne maskinen.** 124 separate runtimekjøringer er avsluttet: 120 PASS og fire GPU-krasj. De består av 110 matrisescenarioer, 13 tillegg og én separat Low-finale. Antallet er et testutvalg med ulike fiksturer, ikke en målt feilrate for vanlig spilling. Ingen av de tre komplette finaleforsøkene nådde sluttittel, rulletekst, sluttfoto og tilbake til meny. Den korte `chapter6event`-prøven besto bare starten av hendelsen og erstatter ikke disse testene. [Uavhengig protokollaudit](/home/tom/scratch-bm/Signal47-playtest-20261025/final-protocol-audit.md).

Åpningen uten mat, de prøvde matvariantene, besøksparkeringen, flere kapitteloverganger, Continue fra siste sikre kapittel 5-sjekkpunkt og anomaly-kontrollene fungerer i de angitte rutene. Det gir et brukbart grunnlag for videre retting, men godkjenner ikke en ubrutt fullnatt. Normal kapittel 2 til 3, menneskelig oppdagbarhet, lytting og fysisk Xbox står fortsatt åpne.

Alle 108 matrisebilder er vurdert på sju kontaktark. Til sammen 58 unike bilder/foto er også faktisk åpnet i full størrelse av de tre bildegjennomgangene og hovedtesten. [Visuell indeks](/home/tom/scratch-bm/Signal47-playtest-20261025/visual-review-index.json). Den samlede bevisindeksen inneholder 124 primærlogger og 160 runtime-PNG-er, inkludert bilder fra de mer omfattende tilleggsrutene; filantallet er ikke i seg selv visuell godkjenning.

Før/etter-verifikasjon er PASS: installasjonens 40 payloadfiler, 40 personlige Saved-filer og 411 testede kilde-/config-/scriptfiler er identiske. Filmoduser og lenkemål i installasjonen er også kontrollert; Git-head er uendret. [Bevaringskontroll](/home/tom/scratch-bm/Signal47-playtest-20261025/preservation-verification.json). Ingen rettelse, ny pakking, installasjon eller spillpublisering er utført. Rapport og oppfølging er skrevet i prosjektets dokumenter.

En kopi av sluttrapporten og råbevisene er bundet i et [SHA-256-manifest](/home/tom/scratch-bm/Signal47-playtest-20261025/final-evidence-manifest.json). De mislykkede forsøkene er beholdt, også der retester besto.

Anbefalt rekkefølge:

1. Rett QA00 og kontroller hele finalen til meny på den samme maskinen. Behold bortføringskrasjet som eget åpent funn.
2. Rett synlig motorvarsel, material-usage/mapping og lesbarheten i nødvendige feltfoto. Profilering må gi grunnlag for videre grafikkoptimalisering.
3. Rett kontrollnavn, lagringsvarsel, tilgjengelighetsvalg og de små tekstfeilene. Avklar kartets krav og notatets sannhetsinnhold.
4. Kjør en menneskelig ny sak uten hopp, med lyd og faktisk kapittel 2 til 3, før ny funksjonsutvidelse kalles ferdig. Bruk observasjonene til å prioritere motivasjon, bevisbrettets repetisjon og personmodellene.

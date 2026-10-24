# Todo for unreal/

## Xbox-kontroller, 24. oktober 2026

- [x] Implementer analog gange/blikk, biltriggere, fast Xbox-oppsett, menyer/panelpeker og separate lagrede kontrollerinnstillinger.
- [x] Bygg og kontroller faktisk kernel/SDL/Slate/gameplay, 1373 selvtester, elleve native opptak og 16 relevante regresjoner. Full verdensrunde stoppet etter Toms beskjed om mindre testomfang.
- [x] Commit/push, PR149 flettet til unreal-port-core og testet kandidat X4 installert: 40 payloadhash, 48 uendrede Saved-filer og verifisert sikkerhetskopi.
- [ ] Menneskelig prøve med Toms Xbox-kontroller. Se GAMEPAD.md for faktisk teststatus.


## Filmisk New Mexico-natur, muligheter lagret 24. oktober 2026

- [ ] Undersøk Quixels Canyons of Utah / Desert Western på Fab som kilde til skannede klipper, steiner og bakke. Samlingen er ikke funnet lokalt; Epic-kontoens eierskap og faktisk nedlastingslisens må kontrolleres. Hele Megascans-biblioteket er ikke automatisk gratis. Bevar lisensbevis og hold tredjeparts råassets utenfor offentlig Git.
- [ ] Start utvalget med gratisoppføringer som Desert Western Cliff Layered XL 14, Canyon Sandstone Rocky Ground og Desert Western Cluster Rock Medium 05. Kontroller pris, format og lisens på selve nedlastingen, og importer bare relevante deler.
- [ ] Form mesaer etter New Mexico-referanser, eksempelvis Chaco/San Juan Basin: lagdelt sandstein, separate klippevegger, eroderte skråninger/rasmark og naturlige overganger mot dalbunnen. Skannede Utah-overflater erstatter ikke korrekt silhuett og skala.
- [ ] Prøv tre avstandslag i gatebiten: detaljert skannet grus/stein, grøfter og vegetasjon nært; belyst 3D-geometri med PBR i mellomgrunnen; enkel fjellgeometri med fotobaserte detaljer og atmosfærisk dis langt unna.
- [ ] Lag en avgrenset prøve med fotografisk kameraprojeksjon / matte painting på fjerngeometri. Kontroller parallax, sideveis spillerbevegelse, strekking/skjulte flater og innbakt sol/skygge ved dagslys, skumring og natt. Bruk vanlig belyst PBR nær spilleren; skill kameraprojeksjon fra verdensjustert/triplanar materialprojeksjon.
- [ ] Vurder Mesh Terrain i en separat prøve dersom terrengverktøyene trengs. UE 5.8.4 har MeshTerrainMode, merket Experimental og ikke aktivert her. Ingen automatisk migrering av fungerende verden; mål faktisk spillerutsikt, kostnad og regresjoner først.
- [ ] Sammenlign samme gateutsikt ved bensinstasjon, suvenirbutikk og radiostasjon fra spillerhøyde i dagslys, skumring og natt. Kvalitetsmål er konseptbildets natur, troverdig dybde, materialer og lys; eksisterende CN5 er fortsatt stilisert.

Kilder fra vurderingen: [Quixels Moab-opptak](https://www.youtube.com/watch?v=ND53AjjPA8Q), [Quixel på Fab](https://www.fab.com/sellers/Quixel%20Megascans?q=arid), [tidligere Fab-/Quixel-rettigheter](https://dev.epicgames.com/documentation/fab/purchasing-and-downloading-assets-in-fab), [Chacos geologi](https://home.nps.gov/chcu/learn/nature/geology.htm), [Mesh Terrain](https://dev.epicgames.com/documentation/unreal-engine/mesh-terrain-in-unreal-engine), [materialprojeksjon](https://dev.epicgames.com/documentation/unreal-engine/texturing-material-functions-in-unreal-engine). Muligheter, ikke implementerte eller visuelt godkjente løsninger.

## Volumetriske skyer, overtatt fra Claude 24. oktober 2026

- [x] Kode f55b760c/04d98231 finnes allerede i integrert natur-/filmversjon; Claudes SKY.md og dokumentlenker er bevart.
- [x] Fullfør cache og sluttdokumentasjon på integrert natur/film/sky-kilde. Claudes ufullstendige opptak bygger på eldre sky-pakke og andre shader keys, og brukes ikke som cachebevis for dagens pakke.
- [x] Kontroller nye pakkede himmelbilder ved dag, kveld, natt, midnatt og daggry, Low-fallback, komet/stjernehull, selvtester og full screenshots.sh før fletting/installasjon.
- [x] Lokalt flettet og installert med 40 kontrollerte payloadfiler, 48 uendrede Saved-filer og backup. Bildene ligger i Signal47 skyer og lys på skrivebordet.
- [x] Dokumenter faktisk cache-/cook-/runtime-binding og målte begrensninger; behold menneskelig gjennomspilling/lytting og lang tids skybevegelse som UNVERIFIED til prøvd.

- [ ] Reduser ordnet rutedithering i enkelte månelyse High/Clean-skyer; sluttbildene dokumenterer dette som åpen kvalitetsgjeld.
- [ ] Undersøk gjenværende oppstartshakking: det ene varme driver-cache-forsøket har 372,56 ms synlig hakk og 57,9 FPS første synlige 10 s. Cachebevis er ikke garanti for jevn oppstart.

## Caliche natur, 24. oktober 2026

- [x] Tre originale Blenderkit, 36 terrengfliser, 12 plante-/steinprototyper med tre LOD-er og revidert lagdelt geologi.
- [x] 33 faktiske spillerhøye bilder ved dag/skumring/natt gjennomgått. Elleve funksjonstester og 1328 selvtester, kilde og ferske native kontroller PASS på fryst CN5.
- [x] Hele 110-scenario screenshots.sh, lokal fletting og installasjon med backup, 40 payloadhash og 51 uendrede Saved-filer. Alle 110 natur-auditer er bevart, inkludert én omkjøring etter et duplisert loggfilnavn. Redigerbare kilder og bildevelgere i Signal47 Caliche natur; se CALICHE_NATURE_PASS.md.
- [ ] Videre fotolikhet: mer naturlig talus/fotrelieff og adskilte berglag, mindre gjentatte busksilhuetter og rikere terrengdrenering. CN5 er et stilisert produksjonsinkrement, ikke nær-foto-godkjenning.

## Caliche konseptpass, 24. oktober 2026

- [x] Ni visuelle runder av den bestilte gatebiten, inkludert fire originale Blender-kilder, PBR/eksporter, kontaktbakke, planter/stein og fjern horisont.
- [x] 42 faktiske spillerhøye bilder ved dag/skumring/natt gjennomgått. Full 110/110 screenshots.sh, elleve runtimeprøver, fotopilot, horisontfallback og native/kildeportabilitet består på samme pakke.
- [x] Lokalt flettet og installert med backup, hashkontroll og uendrede Saved-data. Skrivebordsleveranse med originaler, bildevelger, prioritert modelliste og bevis. Se CALICHE_CONCEPT_PASS.md.
- [ ] Neste modellpass etter denne gateprøven: Mesa Diner, sheriffkontor, motell, vaskeri/fôrbutikk, kino og kirke. Dette er prioritert videre arbeid, ikke utført i denne leveransen.

## Skumlere pilotrunde, codex/anomaly-pilot, 23. oktober 2026

- [x] Sparsomme verdenshint, plausible uavklarte A02-valg, nattefilm ved vaskeriet, rettet/andre flyttet pappfigur og P01 som kommer nærmere ved gange med ett ON AIR-blink.
- [x] Save v3-migrering, ekte bevart film og tekniske selvtester; egne avbruddsrutiner for lys/lyd. Ingen Evening-/voice-endringer eller endring av harde storygrenser.
- [x] Sluttpakke 01d92bd4 med core206afd9b: 1293 selvtester, åtte pilotscenarioer, High/VHS og Low, ekte gammel migrering, 7/7 historieregresjoner og to kameramotiv. Bilder/før/etter visuelt kontrollert. Se ANOMALY_SCARIER.md for bevis og UNVERIFIED-punkter. Content-lenker/OpenOrder ryddet; ingen push/PR/installasjon.
- [ ] Tom prøver lydmiksen, figurens direkte nærvær, fotosporets lesbarhet og frykt med ekte input. Automatikk avgjør ikke dette.
## Takeaway og samlet åpning, codex/evening-runtime, 24. oktober 2026
- [x] Én pose/kaffe kan settes ned og plukkes opp på seks faste steder; plassering, kjøp og ventende svar lagres uten duplikat. Bilplassene følger bilen.
- [x] Valgfri kveldsmat etter D01 ved skrivebord/annex, fra hånd eller bord. Firesekunders overgang, tolv minutter fremover innen tidsbudsjettet, papir/tom kopp etterpå. Lasting under overgangen gjentar ingenting.
- [x] Holdt mat skjules ved kamera/lykt/notatbok og har ingen kollisjon. Gamle arkivbordhandlingen beholdes med korrekt query-høyde; bare én diner-servering tegnes.
- [x] Sluttkilde f83f3dc9/core92dc7a47: bygg/pakking, 1298 selvtester, 33 vanlige bruksforløp, 21 ferske matlastinger, bilbilder, 48 inngangspasseringer, hel burgerrute og tre gamle regresjoner PASS. Bilder gjennomgått, seks pakkehasher uendret. Se EVENING_TAKEAWAY.md. Content-lenker/OpenOrder ryddet; lokale committer, ingen push/PR/installasjon.
- [ ] Menneskelig input og lytting etter Claudes integrasjon med nyere core6614cd90. Denne pakken fryser core92dc7a47; ingen sammenhengende fullnatt eller ny ytelsesmåling påstås.

## Åpningens biler og inngang, codex/evening-runtime, 23. oktober 2026

- [x] Privat wagon med eget karosseri, førermiljø og åpen dør; alle SARO-riggdeler skjult og gjenopprettet på riktig bil. Komplett delt standardmodell bevart.
- [x] Plan SARO 07 på originalpad, samsvarende kollisjon og klar besøksplass; gamle utrygge åpningstilstander normaliseres.
- [x] Reprodusert terskellåsing ved lukking under spilleren, sperret lukking og reparert gammel save før lasterens Walkable-port. Samme posisjon og klokke beholdes.
- [x] Flettet bygg/pakking, 1286 selvtester, 48 vanlige gangpasseringer, fire ferske lastinger pluss gammel terskelsave, hel burgerrute, seks regresjonshopp og alle påkrevde bil-/inngangsbilder PASS. Rapport EVENING_VEHICLE_FIXES.md, pakkekilde 66d2b5a6/core04517414; hasher uendret, lenker/OpenOrder ryddet. Ingen push/PR/installasjon, Claude integrerer.
- [ ] Menneskelig gjentakelse etter Claudes integrasjon. Denne leveransen beviser automatiske bevegelses-/lastescenarioer, ikke Toms neste fullnatt med ekte input.
## Filmfiltre, codex/film-looks, 23. oktober 2026
- [x] Carpenter 35 mm, Desert 16 mm, Super 8, forbedret VHS og Clean; styrke, lagring og tilgjengelighetskobling.
- [x] Separat PictureMode med bevart Vhs-blend og capture-kontrakt; privat materialregenerering.
- [x] Sluttpakke 8ba39556 med core 04517414, 1298 selvtester, ni funksjons-/UI-/diskprøver, 36 kamerabilder og varm High/1440p-kostnad. Claude-gjennomgang, cache-remapping og hasher PASS. Se FILM_LOOKS.md.
- [ ] Menneskelig vurdering av levende korn/flimring og komfort; HDR og SteamOS/Deck. Ny full PSO-matrise for hele den endrede verdenen gjenstår før en slik dekningspåstand.

## Anomaly-pilot, codex/anomaly-pilot, 23. oktober 2026

- [x] N01 med faktisk utskrift mot kveldens service-/sendeplan, korrigert feilvalg og fysisk forespørsel.
- [x] N02 med to faktiske kameravinkler, lampens kontroll og reviderbart resultat.
- [x] A02 med én figur, ekte bevart normalfilm/arkiv, flyttet posisjon og aktiv bildesammenligning; P01 med kort printeretterspill og fysisk fasadekontroll.
- [x] Tidlig kamera/rutinekjøring og save v3-rutinekontrollpunkt; ingen senere kapitteloppgaver løses. Migrering uten oppdiktede eksponeringer; manglende film er eksplisitt utilgjengelig også etter verdenslasting.
- [x] Sluttpakke/1278 selvtester, High/VHS og Low/Clean, feilfilm/migrering, 7/7 kortregresjoner, begge åpningsruter og TV-opptak. Statisk Caliche p95 +0,58 %, ingen konkurrerende tung jobb; fire sluttbilder visuelt kontrollert. Se ANOMALY_RUNTIME.md for avgrensninger. Content-lenker/OpenOrder er ryddet; Claude integrerer, ingen installasjon/push/PR.
- [ ] Menneskelig pilot/Gauntlet: ekte input, lydlytting, forstått kontroll og stigende uro. Ikke erstatt dette med automatiske API-scenarioer. Fullnatt uten testkutt og faktisk spilletid gjenstår.
- [ ] Senere plansteg: Dales kontor/VHS, trafostasjon, bygningsløft og større UFO-/aliensekvenser. Ingen ny anomalikvote for P13.

## Kveldspakkens lyd, codex/evening-runtime, 23. oktober 2026

- [x] 109 takes importert strengt som byteidentisk PCM og SoundWave, med 110 cues og eksakt D61-alias. Andre banker bevart.
- [x] Receiverfilter/radiobuss for host og Dale; posisjonell servitrise, målte captions og uavhengig karakterlyd. Rettet gjentatt panelåpning som stoppet ny stemme, og fjernet misvisende generell opplesningsboks.
- [x] Sluttpakke 9d638a0b: 1258 selvtester, hel burgerrute med 36 fullførte klipp og faktiske nivåer, pause/avbrudd og disk-/verdenslasting ved kjøp/D01, fem bilder pluss telefontranskripsjon, kortregresjon 6/6 PASS. Rapport EVENING_AUDIO.md; Content-lenker/OpenOrder ryddet, ingen generert lyd i Git eller installasjon.
- [ ] Tom lytter på D01_FOOD_002 og D01_BRIEF_005, deretter stemmelikhet/innlevelse og miks i spillet. Fullnatt og VHS er fortsatt ikke levert her.

## Toms trinn 1 og 2, codex/evening-runtime, 23. oktober 2026

- [x] Manusadapter og datadrevne porter for senere faser, knyttet til eksisterende dialog, telefon, radio, notatbok, mål og lagring.
- [x] Spillbar personbil fra sør gjennom Caliche, valgfri diner med mat i hånden, én betalt ordre og atomisk lagret svar, SARO 18:00 én gang og D01 bare på telefon. Den gamle rutinen fortsetter etter overleveringen.
- [x] Kveld/natt/gry på sammenhengende akse, radio-/NPC-tider og kapitler, uttrykkelig legacy-lagring, faktisk utskriftsstempel og bevarte foto-/1947-referanser. Tidskomprimering og gammel/ny-tabell i EVENING_TIMELINE.md.
- [x] Kveldssol, native lys og skumringskurver; ingen minutt-HUD under innkjøringen. Stillbilder gjennomgås i EVENING_SKY.md.
- [x] Pakket kandidat og målrettet regresjon: 1254 selvtester, fire åpningsruter, legacy-migrering, gamle/nye hopp, native telefonbruk og fem siste bilder PASS. Sluttkilde 3d321da1; kildeoverlevering til Claude, ingen installasjon. Se EVENING_RUNTIME.md.
- [ ] Menneskelig gjennomspilling med ekte input, lytting og målt total spilletid. De 109 takesene er senere produsert av Claude-hjelper og integrert av Codex; VHS og verdensinnholdet i senere trinn gjenstår.


## Grafikkpass på port-gfx, 23. oktober 2026

- [x] Viderefør WIP og flett siste core/Props/dokumentasjon uten egne Props-endringer.
- [x] Materialcook uten fallback, 118 profiler/CRT-glass og integrasjonskommando for generert innhold. V5-selvtest (1220) og native fem emitting-slots av/på; V9 terminalfront visuelt PASS.
- [x] Korrigerte 20 + 20 referanser og 40 gjennomgåtte før/etter-par med originalhash/galleri. Props-forskjeller er oppgitt.
- [x] Full 38 × 6 samme-cook PSO-opptak, 4215 stabile PSO-er, sluttcook 599 og runtime-GUID/222 bundled graphics: cachebevis 9/9 PASS.
- [x] Kald/varm sluttpakke, komplette første 10 verdens-/synlige sekunder. READY 11,337/5,594 s, synlige maks 31,585/31,888 ms og ingen rammer over 33,333 ms.
- [x] Sluttpakkens sju tilfeller, 1220-selvtest og 36 fasebilder visuelt kontrollert, samt korrigerte seks grafikkstrømtilstander. Gamle matrix-dark-bilder brukes ikke som av-bevis.
- [x] PreviousNight-kjøring og to identisk-geometri fyllpar visuelt PASS, compare/contrast-control.png og contrast-yard.png, med proveniens i contrast-manifest.json.
- [x] Åtte lengre High/1440p-prøver ved natt/daggry PASS 93,81–106,73 fps, maks p95 18,142 ms. Korte matriseprøver og Low/Ultra er dokumentert, ikke bruk dem som stabilt 60 fps-bevis.
- [x] Sluttregresjon PASS 151,3 s: tittel, pause, notatbok, kjøring, THE EVENT og valgt 30 fps, 285 synlige cap-intervaller med 33,333457 ms i snitt.
- [x] Sluttall og integrasjonskommando i LIGHTING/egen OPP-rad/minne/logg. Content/Audio/Licensed/Free-lenker fjernet med mål bevart, OpenOrder gjenopprettet og syntaks/diffkontroll PASS.
- [x] Kilde og bundlet SPC levert i vanlig commit på port-gfx, med alle grafikkprøver og bildepar dokumentert. Ingen installasjon, push eller PR bestilt.

## Anomaly spotting og byens funksjon (2026-10-23)

- [x] Utdypet design- og gjennomføringsplan etter Toms tilslutning til retningen: `ANOMALY_PLAN.md`. Bare planarbeid, ingen ny gameplay eller installasjon.
- [x] Revidert planen med tidligere kveld, romsligere kapitteltider, trafostasjon og bedre bybygninger etter Toms oppfølging. Start 18:00 og residual 00:13:47 er senere implementert på codex/evening-runtime; se nyere leveransestatus ovenfor.
- [x] Innarbeidet Toms innkjøring med lystig radio, valgfritt matstopp og skiftstart ved SARO 18:00, samt Dale med Kinski-stemme og hemmelig kontor. Bare plan, ingen nye opptak eller modeller.
- [x] Skrevet og strukturert hele manuspakken i `Narrative/EveningShift`: innkjøring/diner, fjernoverlevering og samtaler, kontornøkkel/filer/VHS, trafo og sammenligninger. JSON-/eksportkontroll, opptaksalias og 14 offline forfatterruter er kontrollert.
- [x] Produser 109 unike stemmetakes, importer gjennom streng katalog og koble målt lyd/undertekst til telefon, bilradio og posisjonell servitrise. Produksjon `a4626053`, runtime og testgrunnlag i EVENING_AUDIO.md.
- [ ] Produser det 72 sekunders bevegelige VHS-opptaket fra pakken. Lytt på alle takes, særlig D01_FOOD_002 og D01_BRIEF_005.
- [ ] Trinn 0: mål dagens reise med ekte input, klokke, transport, triggere og passiv venting; behold baseline. Prøv også lang All Night og avreise etter 05:20: dagens ch6 setter avreisen til 05:20, så mulig tilbakesprang må dokumenteres og løses før ny fullnatt godkjennes.
- [x] Trinn 0: tidsreferanseoversikt for 1986-hendelsen og kapittelporter, datoakse gjennom midnatt, kveldsradio/NPC-er og eldre lagringer. 1947-tider og 05:29-finalen beholdt; se EVENING_TIMELINE.md.
- [x] Trinn 1: kveldsstart, tidlig kit/truck og N01/N02/A02/P01 med normalreferanser, aktiv sammenligning, fysisk oppfølging og lagring. Pilot og nøyaktig testgrunnlag i ANOMALY_RUNTIME.md.
- [x] Første Caliche-gatebit med bensinstasjon, suvenirbutikk/interiør, studio og lokal bakke. Installert; 24 spillerhøydebilder og funksjons-/fotoregresjoner PASS. Se Signal47/Tools/calichestreet/acceptance.json. Menneskelig gjennomspilling gjenstår.
- [x] Trinn 1: I01/D01 med innkjøringsbil, radio, valgfri matbestilling, egne tidlige diner-tilstander, lagret ankomst og Dales telefonoverlevering 18:00. Rollen er KUN telefon/radio/walkie; ingen fysisk Dale. Egen opptaksbank er koblet til uavhengig av generell opplesning; nye takes er nå importert og koblet til kanalene, se EVENING_AUDIO.md.
- [ ] Trinn 1: prøv begge delstrekninger, migrering, sen ankomst og fem observerbare Gauntlet-mål. Merk testkuttet; ikke kall prøven hel gjennomspilling.
- [ ] Trinn 2: ny trafostasjon i ørkenen med avstikker, T01/T02, kontrollresultat, lys/lyd og lagring. Skill vanlig feil fra senere avvik; bevar STATION 01 og K3s rolle.
- [ ] Trinn 2: D02, fysisk reservenøkkel til Dales eksisterende kontordør, private utklipp/filer og faktisk bevegelig VHS/TV med lyd, pause/strøm/lagring. Ett funn gir etterprøvbar feltoppgave; ikke erstatte Nora, avisavsløringen eller P13.
- [ ] Trinn 2: gjør kvittering, kart, telefon og vitner til etterprøving; prøv All Night fra 04:10 og lysbølgen før 04:55. Ferdigstill bedre diner, sheriffkontor og vaskeri med nødvendige rom.
- [ ] Trinn 3: fjern himmelform, sen egen alien og større UFO-observasjon; samordne eksisterende glimt/bortføringer. Resten av byens utvendige bygg får samme kvalitetsløft, med bevart funksjon og nye kveld/natt/gry-bilder.
- [ ] Trinn 4: hele natten med ekte input, riktig klokke og kapittelflyt, lagring, nødvendig regresjon og målt ytelse. Fullnatt og opplevelsesmål er foreløpig UNVERIFIED.


## Native lys og skygger (2026-10-22)

- [x] Erstatt materialenes flood/hemi og gamle projeksjoner med ekte lamper; behold historienes tilstand og delte lyskilder.
- [x] Lommelykt og billykter med skygger, volumetrisk tåke, Lumen, svak måne/skylight og levende daggry. Fast eksponering og VHS/Clean, uten lysbryter mellom Web og Unreal.
- [x] Lokalt editorbygg og målrettede native scenarier fra SARO til den gamle veien. Side-ved-side-bilder og detaljer i LIGHTING.md.
- [x] Taklys fra faktiske armaturer i rom/korridor/annex/lab, synkron emissive, skrivebordslamper fra skjermåpningene og egen grønn glød foran hver registrerte skjerm.
- [x] Varme, skyggekastende array-spotter fra de elleve eksisterende feltarmaturene. Grå diffuserflater og Ops-ghost under strømbrudd rettet.
- [x] High/1440p med TSR fra 60 %: rom 73,4 fps, gård 95,5 fps, p95 15,330/11,527 ms uten andre tunge prosesser i målevinduet. Low/Clean 90,0 fps i rommet, Ultra 39,2 fps i gården. Detaljer i LIGHTING.md.
- [ ] Pakket bygg og menneskelig gjennomspilling med de nye lysene. Standardutgaven er ikke oppdatert av dette arbeidet.
- [ ] Kjør det isolerte materialsteget ved integrasjon med eldre generert Content, før kandidat eller pakket utgave brukes.

## Prøven av prologen

- [x] Prosjektet bygger, modellene er importert, og den pakkede builden starter med kontrollrommet, gården og himmelen (2026-10-21).
- [x] Alle fjorten testkjøringene gjennom med verdenen fra nettversjonen (2026-10-21).
- [ ] Tom går rundt i rommet og ut på gården og sier om prøven holder: lys, stemning, modellenes skala og retning.
- [x] VT323 og Special Elite som i nettversjonen (2026-10-21).
- [x] Verdenen hentes fra nettversjonen (Tools/webexport), så modellene står som der (2026-10-21).
- [ ] Bildet sammenlignet med nettversjonen fra de samme stedene: rommet, gården, antennene, vinduet, racket.

## Hvis prøven holder (hele konverteringen)

- [ ] Prologen spilt av et menneske fra 23:41 til sluttkortet.
- [ ] Historien som JSON felles for web og Unreal, slik at tekst og valg ikke må skrives to steder.
- [ ] Feltkameraet og fotolaben.
- [x] Lyd og musikk fra web/public (2026-10-21).
- [x] `core/Audio.ts`, `signalVoice.ts` og `ambience.ts` portert på en Web Audio i C++, kryssjekket mot Chromium (2026-10-22).
- [ ] Natta ute i gang som i main.ts: `StartNight()` etter `StartRoomTone()`, `NightLife(dt, spillerens posisjon, klokka)` hver ramme, `SetSpace(room|yard|lab)` ved områdebytte, `Suspend` i pausen, volumene fra innstillingene.
- [ ] Prologen spiller tonene med `Beep(...)` og nettversjonens tall i stedet for filene fra make-synth.py, og radioen i høyde 1,45 som i Prologue.ts.
- [ ] ReadAloud og RecordedDialogue (stemmebankene under /Game/Audio/Voice, katalogene i Content/Audio/Raw/voices.json); `decoder.ts` trenger en ConvolverNode.
- [x] RadioVoice, radio.ts, tripRadio.ts og score.ts med kilde for lange opptak, volum, innkobling og selvtester (2026-10-22).
- [ ] Radio-/musikkporten prøvd i samlet pakket bygg med hele screenshots.sh og menneskelig lytting/input. Koble øvrige nattsystemer til Score.Held og samme Main.TownSeen; se RADIO_PORT.md.
- [ ] Lagring ved kontrollpunkter.
- [ ] Kapittel 1 til 6 (web/src/story) og områdene. Verdenene er med i eksporten (AS47Web::Enter).
- [x] Kjøringen (web/src/drive) og turene i World.ts (2026-10-22). Gjenstår: eksporten kjøres på nytt for `drive` i manifestet, lydene laget med Web Audio (dekk, vind, startmotor, ferist), lyktenes mønster (headSlots) i flomlysmaterialet, lyktene på dineren, byen og motellet (shareLights), radioen.
- [x] Glødepunktene blinker som i glow.ts (2026-10-22, `S47Glow`).

## Kapittel 2 (2026-10-22)

- [x] Kapittel 2 portert (S47Chapter2, S47Annex, S47PhoneScripts, S47Drawings2, S47Panels2), selvtest og gjennomkjøring uten verden.
- [ ] Dørene (Doors.onChange) videre til kapitlenes `DoorChanged`, som Prologue.doorChanged.
- [ ] Lagringen kobles til `US47Chapter2::OnSave` og `Host->LoadCase`.
- [ ] Kapittelkortets ring (`bHoldRing`, `RingOut`, `RingAgain`, `RingShown`) når kortet er portert; kapittel 1 sin slutt kaller `US47Chapter2::AfterChapterOne(method)`.
- [x] Bryterbordets linjelamper og opptakerens papirstrimmel (archive047.ts): i export.js (2026-10-22); kjør eksporten på nytt for å få håndtakene (C++ finner dem uten).
- [ ] S47Drawings og S47Drawings2, S47Panels og S47Panels2 slås sammen.
- [x] Lagring ved kontrollpunkter, tittel, pause og notatbok (2026-10-22, `S47Saves`, `S47Flow`).
- [ ] Kapittel 1 til 6 (web/src/story), kjøringen (web/src/drive) og områdene. Verdenene er med i eksporten (AS47Web::Enter).
- [x] Glødepunktene blinker som i glow.ts (2026-10-22, `S47Glow`).

## Kapittel 4 (2026-10-22)

- [x] Kapittel 4 portert (S47Chapter4, S47Room6, S47Motel, S47Walkie, S47CB, S47NpcDialogue, samtalepanelet i S47Panels), selvtest og kjøringer i -game uten pakket build.
- [ ] Med kapittel 3: P11 og P12 trenger nålene P08, e10.carrier og hut.rx på saksbrettet (kapittel 3 sin regelbok). Kapittel 3 sin slutt kaller `US47Chapter4::BeginChapter4(nullptr)`, og kapittel 3 sin `truckTrip` går foran i `US47World::TruckChoice`.
- [ ] Kjør `Scripts/audio-raw.py` (eller `import-audio.sh`) på nytt i hovedkopien, så replikkene i rom 6 har stemme (rollestemmene er nå 50).
- [ ] Hjelperne fra main.ts: `Taken` (abduction.ts), `OfficePhone` (officePhone.ts), og at CB-anropet venter på bortføringen og truckTaken.
- [ ] Lyset fra vinduene på gangveien (windowLight.ts) og papirbildene (`cardField`, `letterPaper`) i dokumentvisningen.
- [ ] Continue i rom 6 prøvd med en ekte lagring.

## Etter lagring og flyt

- [ ] Kjør screenshots.sh med den pakkede builden (title, pause) og se tittelkameraet over gården og miniatyrbildene i lagringene med verdenen på plass.
- [ ] Saksbrettets oppsett i lagringen: sett `M->Flow->SaveBoard` og `LoadBoard` til CaseBoard sin Save()/Load().
- [x] Feltkameraet og rullen i Kit-fanen: `M->Flow->HaveCamera` og `RollText` (2026-10-22).
- [ ] Musikk- og radionivået (Music, Radio) inn i lydmodulen når den har egne busser; volumet virker allerede.
- [ ] Feltkameraet og rullen i Kit-fanen: sett `M->Flow->HaveCamera` og `RollText` når kapittel 1 og albumet er portert.
- [x] Musikk- og radionivået (Music, Radio) inn i lydmodulens busser, inklusive tittellåten (2026-10-22).
- [x] Bildevalg erstattet av filmprofilene i FILM_LOOKS.md; gamle `heavy`-valg normaliseres til forbedret VHS.
- [ ] Fortelleren (Serling) til åpningskortet og kapittelkortet når stemmene er portert.

## Nattens lag (2026-10-22)

- [x] Loggen, kurven, nattens hendelser, ropene, glimtene, lommelykta og albumet (`US47Night`), med lagringen.
- [ ] Krokene i `FS47NightHooks` settes når systemene porteres: bortføringen, veikanten (stillheten), byens hendelser, skrittene på taket, motellets fjernsyn, lysene på B, Earls spole, SARO mørk, kontortelefonen, arkivtelefonen, matrisen tilbake, papfiguren, bilen på B, tatt i bilen, opptaket, morgenen, den niende kua, stjernehullet og bilradioen.
- [x] Det nærmere blikket (zoomK, `S47Phones`, 2026-10-22). Fotskrittene (`player.onStep`) gjenstår.
- [ ] Glimtets og albumfigurens malte omriss (runde 14) og albumets bilder (runde 15) er ikke i eksporten; figuren fra koden brukes.
- [ ] Lommelyktas mønster (core/cookies.ts) og fall-off: nå Unreals invers kvadrat med intensiteten avstemt 4 m ut.

## Telefonene, arkivet og opptegnelsene (2026-10-22)

- [x] Arkivtelefonen og matrisen den vipper, kontortelefonen, skrittene på taket, feltboka og konvolutten, blikkene og det nærmere blikket, tråden (`US47Phones`).
- [ ] Krokene fra andre porter i `US47Phones`: `Reel047` (reel047.ts på dekket), `DecoderPlaying`/`DecoderStop` (Decoder.ts), `Dark` (saroDark), `CbBusy`, `AbductionCount`, `DishRogue`/`DishBack` (dishesFollow, arrayBack), `Frames`, `FbGet`/`FbSet` og `StationDrawer` (kapittel 3), `Foretell` (beitet og lysene på B), `StarHoleExamine`.
- [ ] Kapittel 4: `OfficePhone` på kontortelefonens proxy (`court:officePhone`) spør `M->Phones->OfficePhone` først.
- [ ] Kapittel 2s slutt venter på skrittene: settes kapittel 3s begynnelse som ny `OnEnd`, går den gjennom `M->Phones->Ch2End`.
- [ ] Opplesningen av et nærmere blikk etter 650 ms (Serling) når fortelleren er portert.
- [ ] Dishenes røde lys blinker i takt mens de er vippet (DishArray.beaconPhase) når glødepunktene blinker.
## SARO sine egne hendelser (2026-10-22)

- [x] SARO mørk og telexen fra Hondo Valley, matrisen tilbake, antennene som følger, lysene på B, spole 047, Earls spole og opptaket (`US47SaroNight`), med lagringen.
- [ ] Bortføringens port setter `US47SaroNight::Abduct` (stage, warn, backAt, data.last, overPhase, recordings, holdRecord, data.dishes) og bruker `FS47Walker` fra S47Recording.h for den som står der spilleren ble tatt; til da spilles varselet for antennene av testflagget.
- [ ] Radioens port setter `BoyCallTimes`, `CallerVoice` og `BoyVoices` (Earls spole), og musikken holdes av `ReelTapeBusy`, `SaroDarkBusy`, `ArrayBackBusy` og `RecWalkBusy` (musicHeld).
- [ ] Arkivtelefonens port spiller `Reel047` (update, play, reset) og setter `TipMoving`/`TipHolds` (archiveDishes.ts); feltboka setter `LineLights->bAnyWay`; det nærmere blikket setter `Zoom`.
- [ ] Hjørneklokka i sekunder: én kjede i main.ts sin rekkefølge (abduction.warnClock, saroDark, archive, ch6) i stedet for hver sin `ClockSeconds`.
- [ ] Telexrullen på maskinen (ControlRoom.setTelexRoll) tegnes ikke; papiret og loggen har meldingen.
- [ ] ConvolverNode i S47WebAudio: reverbet i decoder.ts er en kambank som står i for den.
- [ ] Glødepunktene utenom antennenes røde lamper blinker ikke ennå.

## Caliche, veien og motellets natt (2026-10-22)

- [x] C++-port av Town/TownEvents, Roadside, Herd/Pasture, NinthModel/NinthCow, MotelTv, MorningHolds/DinerCooler, Cutouts, Caliche og Diner med main.ts-koblinger, lagringsfelt og testflagg.
- [x] Caliches RecordedDialogue/NpcDialogue/EarlDialogue, innspilte replikkøer, replay og stopp. Generell ReadAloud/Serling gjenstår hos fortellerporten.
- [ ] Regenerer webeksport og import for byens delte lamper/glødepunkter, fjernbilens lys, townSlots/neonSlots og herdets beamLit. Implementer pointScreen i native billboards.
- [ ] Koble LostNights til Caliche.Reel og kapittel fem til TownLayer.Diner. Bygg og prøv kapittel tres FreeShot etter integrasjon; kapittel 3/5 finnes ikke i grenens utgangspunkt.
- [ ] Samlet pakket bygg, hele screenshots.sh, Continue fra disk, ekte input, menneskelig lytting og sammenligning av de regenererte bildene mot nettversjonen. Se TOWN_PORT.md.

## Verdenens eget liv (2026-10-22)

- [x] Glødepunktene, floraen og vinden, den levende himmelen og horisonten, lyset i lufta, vinduslyset, nattelivet, Ultra-lysene og de delte lysene (`US47WorldLife`).
- [x] Toppunktfargene i importen og de nære mesaene uten tåke (hvite kanter ved STATION 01).
- [ ] Eksporten og importen kjøres på nytt i hovedkopien (`Tools/webexport/import.sh`).
- [ ] Krokene i `US47WorldLife::Hooks` settes av kapitlene: `SkyLift` og `SunElev` (kapittel 6, soloppgangen), `SaroDarkAdapt` (SARO mørk), `MorningBusy` (morgenen), `LifeQuiet` (stillheten).
- [ ] `S47ArchiveDishes` bruker `FS47GlowPoints::SetPhase` og `HoldAt` for de vippede dishenes røde lys (DishArray.beaconPhase).
- [ ] `US47Night::Dawn` bruker sin egen formel; bør lese `Sky->Dawn`.
- [ ] Resten av World.update: neonflimmeret på motellet og dineren, byen, beitet og flokkene.
- [ ] Bildevalget Off: stjernene, båndet og stjerneskuddet tegnes uten tonekurve i nettversjonen (ShaderMaterial uten tonemapping_fragment); her går de gjennom M_WebTone som med VHS.
- [ ] Floraen: månelyset skaleres ikke med fSky (bare halvkulelyset), og spotlysenes fFace er ikke portert. Low-nivåets kortere tegneavstand for støv og hvilende tumbleweed er tilnærmet.

## Integrasjon etter bølge 4 (2026-10-22)

- [ ] Kapittel 5 (hjelper) og kapittel 6 (Codex, `port-ch6`) flettes inn; kapittel 5 sin slutt inn i `BeginChapter6`, `ch6.clockShow` sist i hjørneklokka, `US47WorldLife::Hooks.SkyLift`/`SunElev` og `US47Away` sine `Ch6*`-kroker.
- [ ] Opptaksvandringen (`S47Recording`) og bortføringens `FS47JacketWalker`/`Recordings()`: to figurtyper; vandringen bruker sin testopptakelse til de deler én.
- [ ] `US47Away::AfterAbduction` er ikke satt: stationTaken, «station passed», opptaksvandringen og parabolene steppes hver for seg (kapittel 3 og `US47SaroNight`), ikke i bortføringens blokk.
- [ ] Arkivets tippede paraboler: `FS47GlowPoints::SetPhase`/`HoldAt` for de røde lampene (verdenens liv).
- [ ] Byens fjernbil: fast pikselstørrelse (`pointScreen`) for billboards.
- [ ] Kapittel 4: papirkunst `cardField` og `letterPaper` mangler i WebData/art; WindowLights på gangen, readAloud og Earls stemme.
- [ ] CRT-gamma (dobbel koding), Serling-fortelleren, jakka «Cold out», kameraet senket på kortet (fra kapittel 1-hjelperen).
- [ ] `US47Night::Dawn` bør lese `Sky->Dawn` i stedet for egen formel.
- [ ] Pakket bygg: full `Scripts/screenshots.sh` etter ny eksport og import, og en gjennomspilling med ekte input og lyd.

## Kapittel seks og avslutningen (2026-10-22)

- [x] Chapter6.ts og Ending.ts i C++, med main.ts sin finale og testhoppene chapter6, chapter6:lastmile og chapter6:line.
- [x] Siste mila, CB-klokka, key/door og kamerastillingene med selvtester; native scenarier ved avreise og på linja, THE EVENT og kamerauttrekket fram til kontrollert tittelbilde.
- [ ] Integrer kapittel fem sin OnEnd og dinerlagring med BeginChapter6; prøv Fortsett fra denne lagringen etter avslutningen.
- [ ] Bind daggry-/soloverstyringene til verdensportens dynamiske himmel og kontroller mesa, soloppgang og stjerner visuelt mot nettversjonen.
- [ ] Pakket bygg, hele screenshots.sh, menneskelig input og lytting; øvrige uferdige motorlydgrafer hører til motorporten.

## Toms ideer (2026-10-22)

- [ ] Kassettspilleren: bytte den egenlagde tape deck-modellen med en bedre modell. NB: opptakeren i «Utstyr lyd UFO» på skrivebordet er pakket ut av «They Are Here»-demoen (UModel); den kan ikke brukes uten å kjøpe originalpakken (Horror Engine). Den kjøpte Vintage Liminal-pakken har kassetter og en Broadcast VCR, ingen kassettspiller.
- [ ] Tonene: en kombinasjon gir en annen låt og låser opp noe (hemmelighet, hint eller spor). Poenget må bestemmes.
- [ ] Ut av bilen overalt (feil) og hopp: under arbeid.
- [x] Egen alien laget i Blender med nye teksturer, 50-beins rigg og idle: `/home/tom/Desktop/Alien - Egen modell/`. FBX/GLB og UE 5.8.4-import består. Gammel modell er urørt referanse. Integrasjon og prøving i pakket spill gjenstår.
- [ ] Terminalene i verden: kameraet zoomer inn på den fysiske skjermen, og skjermbildet vises på skjermmodellen (som i Black Map). Under arbeid for mottakerkonsollen og driftsterminalen.
- [ ] Grafikk: mer detaljerte modeller, bedre lys og skygger (neste store steg).


## Unarius, overlevert til Claude 22. oktober

- [ ] Fortsett codex/unarius-tv fra TV_PROGRAMME.md: bruk siste bygde binærfil med tidsrettelsen, verifiser faktisk bevegelig film på begge TV-er over tid, koble filmlyd om ønsket, prøv fallback og 47-sekundershendelse/opptak/lagring på nytt, integrer og pakk før standardutgaven erstattes.
- [x] Separat egenprodusert Blender-alien er levert i `Desktop/Alien - Egen modell`, med importprøvde Unreal-assets. `Desktop/Alien` er urørt referanse; den nye figuren er ikke integrert i spillet ennå.

- [x] KELVORA retro-TV og slitt radio plassert i Unreal, 22.10: motellkontoret, rom 6 og SAROs arkivskap. Levende Unarius-film og lyd, av/på, radio og TV-hendelsens lagring verifisert. 1218 selvtester og ni målrettede scenarioer; installasjonen bak skrivebordssnarveien oppdatert med lagringer bevart. Se `Signal47/Tools/kelvora/README.md`.
## Etter Toms utelys-/fotolabprøve, 22. oktober 2026

- [x] 30 utvendige armaturspotter ved SARO og motellet, navngitte labdrivere og kopling til innflettet fremkallingsregel; bygg, 1218 selvtester og målrettede bilder. Se `LIGHTING.md`.
- [ ] Claude gjennomgår `revision3/compare`, pakker i avtalt kø, prøver lukket dør, rød fremkalling, forstørrer, tåket kopi/gjentakelse og normal lysretur med ekte input; SARO-kutt/tilbake og Room 6s lys.
- [ ] Ny High/1440p `stat unit` med én tung jobb om gangen etter dette utelyspasset. Forrige lysleveranses tall er bevart som historikk, ikke målt på nytt her.


## Radiostasjon og array, 2026-10-23 kl. 23:53

- [x] Blender-kilder, realistisk antenne på alle 27 plasser og radiostasjon med interiør integrert, verifisert og installert. Se RADIO_ARRAY.md for bevis og kildekontrakt.

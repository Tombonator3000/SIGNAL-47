# Spillbar kveldstart og kveld/natt/gry

Arbeid på `codex/evening-runtime`, fra `96383c9f`. Codex eier trinn 1 og 2, også spillogikken. Claude integrerer. Den tidligere runtimeleveransen er siden flettet og installert av Claude. Codex har ikke oppdatert `~/Games/Signal47`, pushet eller laget PR i denne leveransen.

## Runtime

Ny natt fra tittelen begynner i personbilen på den eksisterende riksveien, sør for Caliche. WASD kjører, R slår lokalradioen av/på og E går ut når bilen står. Mesa Diner er valgfri; bestill hos servitrisen eller ved disken. Burger/sandwich, vann, spising, takeaway og avslag går gjennom manuspakken. Mat, tallerken, kopp og pose vises i verden/hånden. Personbilen har egen utvendig modell og parkering; den gjenbruker eksisterende kjøre- og dashbordrigg. SARO 07 beholder eget arbeidsliv og kilometerteller.

Faktisk parkering på SAROs pad setter `intro_arrived` og starter skiftklokka 18:00 én gang. Telefonen i kontrollrommet ringer. D01 spilles bare i telefonkanalen; Dale finnes ikke som fysisk figur. Samtalens gren og neste linje kan gjenopptas. D01_BRIEF legger overleveringsdokumentene i notatboken og slipper den eksisterende prologens rack-/kalibreringsforløp videre.

`FS47EveningPack` kompilerer den deterministiske JSON-eksporten direkte. Det finnes ikke en håndskrevet kopi av manusdialogen. Bare navngitte trigger-scener kan startes; choice-scener krever et gyldig valg. Betalingen og den ventende serveringsscenen ligger i samme serialiserte tilstand. Dokument-ID-er tildeles én gang. Eksterne resultatflagg settes fra faktisk runtime, og kan ikke skrives av manusets effekter. Senere faser er datadrevne porter, ikke ferdig kontor-/trafostasjon-/VHS-spill.

Lokalverten i innkjøringen har egen separat mottakerkø, rolle/eksakt tekst og 45 sekunders musikkrom mellom innslagene. De 109 takesene er nå produsert av Claude-hjelper og koblet til runtime av Codex; kanalruting, målt underteksttiming, import og nye lydtester står i [EVENING_AUDIO.md](EVENING_AUDIO.md). Et avbrutt opptak starter gjeldende linje fra begynnelsen, mens manuspeker og kjøp beholdes. Ingen Earl-opptak erstatter verten. VHS gjenstår som egen produksjon. Resten av testresultatene nedenfor beskriver den tidligere runtimeleveransen før take-integrasjonen.

## Tid, lys og gamle saker

Se hele tabellen med gamle og nye hendelsestider i [EVENING_TIMELINE.md](EVENING_TIMELINE.md). Ny tidslinje har residual 00:13:47, kapittelporter 02:20/03:20/04:10 og uendret finale 05:29:00 til 05:29:47. 1947-opptaket har sin egen uendrede tid.

| Hendelse | Gammel sak | Ny kveldssak |
|---|---|---|
| Spillstart | 23:41 i kontrollrommet | Sen ettermiddag på veien; ingen minutt-HUD |
| Dale / overlevering | Skiftpapir datert 23:20 | Telefon etter faktisk SARO-parkering 18:00 |
| Survey-fadens slutt | 02:13:41 | 00:13:41 |
| Residual | 02:13:47 | 00:13:47 |
| Kapittel 2, standard fixture | 02:55:12 | 00:55:12; minst 40 minutter etter array-hendelsen |
| Kapittel 3, tidligste overgang | 03:40 | 02:20 |
| Kapittel 4, tidligste overgang | 04:35 | 03:20 |
| All Night, tidligste overgang | 05:00 | 04:10 |
| Earl / Night Line | 00:00 til 04:00 | Uendret; egen kveldslokalvert før midnatt |
| Avreise / siste CB | 05:20 / 05:28:13 | Samme tider, med fremoverrettet tidsbudsjett |
| Finale | 05:29:00 til 05:29:47 | Uendret |
| Historisk 1947-opptak | 02:15:10 til 02:17:47 | Uendret, egen historisk klokke |

Vanlig aktivitet leder til tidskomprimering i den eksisterende survey-faden; spilleren må ikke vente seks virkelige timer. Ved svært lang aktivitet før signalet holdes fortellertiden ved 00:13:41 frem til survey-utløsningen. Senere kapitteloverganger går bare fremover. Ny natt holder fortellertid ved 05:20 ved svært lang frivillig utforskning, og siste kjøretur bruker avstand, slik at ingen avreise eller rygging ruller klokka tilbake. Disse fristene er komprimeringsregler. Total spilletid er ikke målt.

Lagringsdata har `v=3`, med eksplisitt `timelineVersion=2` for nye saker. Saker uten tidslinjeversjon blir uttrykkelig merket `timelineVersion=1` ved ny lagring og beholder den gamle 02:13-historien. Fotografier og lagrede dokumenttekster omskrives ikke. En eldre feil laget utskriftens hentetid på nytt ved lasting; nye saker lagrer nå faktisk `printoutAt`. Gamle saves hadde aldri lagret dette tidspunktet og får derfor en uttrykkelig «TIME NOT RECORDED / reconstructed legacy copy», fremfor en oppdiktet originaltid. En legacy-merknad forklarer dette i notatboken. Åpningen kan lagres før, under og etter bestillingen og under overleveringen. Kort transport/parkering fullføres før disk-snapshot. Den gamle prologens survey har fortsatt kontrollpunkt ved residualen.

Ettermiddag, vestsol, skumring og blåtime bruker den levende himmelen og native sol/skylight. Morgenkurven `Dawn()` beholder sin tidligere betydning for historien. Felles visuelle dagslysporter bruker `Daylight()`. Detaljer og kurver i [EVENING_SKY.md](EVENING_SKY.md).

## Kontrollgrunnlag

- Editorbygg: PASS etter retting av UE 5.8 JSON-nøkler og kompilatorfeil. Siste bygg 13 inkluderer alle rettelsene, uten UBA.
- Linux Development-pakke: PASS, siste BuildCookRun 107,52 sekunder, ingen materialfallback ved cook. Sluttkandidaten er bygget fra `3d321da1`. Binær-/pakkefilene og krediteringen er hashet i `candidate-verified-manifest.json`. Den første frosne kandidaten fra `d3000904` er beholdt med egen manifest for regresjonsgrunnlaget.
- Siste pakkede `-S47SelfTest`: PASS, 1254 ytre sjekker; pakktesten inneholder dessuten 60 interne kontrollpunkter. Dette er API-/tilstandstester, ikke menneskelig gjennomspilling.
- Pakkeeksport `build_pack.py --check` og stemmekatalogens 12 isolerte kontroller: PASS. Katalogtestene brukte midlertidige syntetiske PCM-filer, ikke produksjonsstemmer.
- Innkjøring og faktisk SARO-parkering: nye bilder er visuelt gjennomgått, med «LATE AFTERNOON» uten minutt-HUD og varmt 18:00-lys uten stjerner. Første direkte gåtest krysset rekkverket og ble rettet til den faktiske åpningen. Se også lysrapportens avgrensning.
- Burgerens betaling, ventende svar og spillerposisjon: PASS gjennom faktisk disk-snapshot, ny `FS47SaveStore` og full `Flow.StartGame`-lasting. Ett kjøp, ingen duplisering, ingen ch5. Bestillingsbildet viser dialog og 3D-mat i hånden.
- Pakket direkte rute: PASS COMPLETE og normal exit, 183,1 sekunder aktiv kontrollrute og 2647,4 kjørte meter. Fysisk kjøring/gåing, engangsankomst, telefonens disk-/verdenslasting, tre dokumenter én gang og frigitt gammel rutine er kontrollert. Dette er utviklerkontrollere og API-dialog, ikke menneskelige tastetrykk.
- Første komplette editor-burgerrute bestod selve reisen, men feilet ved avslutning fordi `Stop()` traff allerede fjernede verdensnoder. `fc8d839b` retter livsløpet; den pakkede direkte ruten avslutter rent. Første 18:35-bilde viste for mange stjerner; `9e7a921a` retter dempingen, og pakket retak er visuelt PASS.
- Alle fire pakkede åpningsruter på `d3000904`: PASS COMPLETE, normal exit og ingen fatal-/assert-feil. Burger og sandwich inkluderer disk-/verdenslasting etter betaling, og alle fire inkluderer lasting under D01. SARO-pickupens lagrede `odo.m` og `odo.x` er fortsatt null etter innkjøringen. Se `route-results.json` i evidensmappen.
- Pakket regresjon fra `d3000904`: PASS for legacy/ny residual, faktisk signalutløsning på begge tidslinjer, gammel utskrift, nye kapittel 3/4/5, THE EVENT på begge tidslinjer og vanlig New night. Telefonens sikte-/brukspunkt er også PASS fra lagret ankomst. Første sikteoppsett var ugyldig fordi parkeringsoppsettet flyttet kameraet etter `S47Face`; retaket bruker en uttrykkelig lagringsfixture og samme native trace/Use som spilleren.
- Legacy-migrering gjennom faktisk meny og disk: PASS, ni auditpunkter. En isolert v2-fixture uten `timelineVersion` lastes og lagres som state v3 / timeline 1. Klokke, bildets 02:13:47-stempel, ID/tekst/sted og bildefilens bytes beholdes; legacy-merknad legges til, ukjent utskriftstid forblir ukjent. Bildet i denne syntetiske testfilen er en eksisterende runtime-PNG, ikke en påstått kameraeksponering. Se `legacy-migration-audit.json`.
- Siste bilde av besøksbilen avdekket at den første standardposisjonen `(2,0,8)` lå inne i annexets sokkel. `1f8befc3` flytter den til grusen `(4.5,-0.62,14)` og retter samme presentasjonspose ved lasting. Retak viser begge biler atskilt og besøksbilen fri av sokkelen. Klokke, papirer, bilder og servicepickup endres ikke.
- Sluttkandidat `3d321da1`: PASS, 1254 selvtester og hele burgerruten igjen, 223,7 sekunder kontrolltid, inkludert betaling/disk/verdenslasting, engangsankomst, telefonlasting, dokumenter og normal exit. Den gamle service-returmeldingen med 17:20 er undertrykt under innkjøringen; parkeringsretaket viser bare 18:00. Alle fem siste bilder er åpnet og visuelt gjennomgått. Ingen utestående FAIL i leveransens funksjonskontroller; det ugyldige sikteoppsettet og de rettede visuelle feilene er beholdt som historisk evidens.
- Menneskelig gjennomspilling, lytting og målt total spilletid: UNVERIFIED. Faktisk film/trafo/kontor/anomaly spotting hører til senere trinn.

Evidens ligger i `/home/tom/scratch-bm/Signal47-evening-runtime/`. Sluttkandidaten er `candidate-verified/Linux/Signal47.sh` i den mappen. `candidate` og `candidate-final` er tidligere frosne testpakker. Kilde, editor-kandidat, pakket kandidat og installert standard holdes atskilt. Alle tester bruker isolert `UserDir`. Kandidaten er ingen installert spillutgave eller fullnattgodkjenning. De midlertidige Content-lenkene er fjernet med målassetene bevart, og genererte OpenOrder-endringer er gjenopprettet.

| Pakket rute | Kontrolltid | Kjørte meter | Resultat |
|---|---:|---:|---|
| Direkte | 183,1 s | 2647,4 | PASS, uten mat |
| Burger med hjem | 223,6 s | 2651,5 | PASS, kjøpt/tatt, ikke spist |
| Sandwich i dineren | 223,7 s | 2651,5 | PASS, kjøpt/spist, ikke tatt |
| Bare vann | 221,1 s | 2651,5 | PASS, ingen matkjøp |

Tabellen gjelder den frosne `d3000904`-kandidaten. Kontrolltid gjelder den automatiserte ruten med korte API-dialogpauser, ikke menneskelig lesetid eller målt spilletid. Oppstart og nedstenging er utenfor disse tidene; kjørelengdene er kontrollens posisjonsanslag.

Alle fem påkrevde bilder er faktiske 1600 × 900-opptak fra samme sluttkandidat (`3d321da1`), åpnet og visuelt gjennomgått. `required-images.json` inneholder hasher og proveniens.

| Utsnitt | Bilde |
|---|---|
| Innkjøring, uten minutt-HUD | [commute](/home/tom/scratch-bm/Signal47-evening-runtime/verified/burger/burger-commute.png) |
| Diner, betalt bestilling og mat i hånden | [diner-order](/home/tom/scratch-bm/Signal47-evening-runtime/verified/burger/burger-diner-order.png) |
| Faktisk parkering og engangsstart 18:00 | [parking](/home/tom/scratch-bm/Signal47-evening-runtime/verified/burger/burger-parking.png) |
| D01 på telefon ved 18:00 | [phone](/home/tom/scratch-bm/Signal47-evening-runtime/verified/burger/burger-18-phone.png) |
| SARO i skumring, fremoveroppsett 18:35 etter D01 | [dusk](/home/tom/scratch-bm/Signal47-evening-runtime/verified/burger/burger-dusk.png) |

## Integrasjon og kilder

`unreal-port-core` er flettet inn igjen i `f0fe3293` og `704b0d21`, sist med `ac72d336` og Claudes oppdaterte bensinstasjonslys. Bensinstasjonens modell, `Config/Props/town.json` og `S47Props` har bare innkommende mergeendringer, ingen ny implementering fra denne oppgaven.

Prosjektbiblioteket ble søkt først og én gang bredere. EngineWire-profilen, Quodsolers UE-skillsprofil og Scenario Labs sin lys-/renderprofil ble lest av de respektive arbeidsdelene. De gir generelle prinsipper og er UNVERIFIED som gjenbruk her; ingen adapter eller uprøvd hjelpeskript er hentet inn. Implementeringen bruker prosjektets egne systemer. GitHub-kontroll 23. oktober viste `PRIVATE`, standardgren `main`, og ingen åpne PR-er mot `unreal-port-core`. Ingen synlighet er endret.

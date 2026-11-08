# Gjeldende beslutninger

**Status:** Gjeldende. **Gjelder:** Retning og arbeidsregler for SIGNAL / 47. **Sist kontrollert:** 6. november 2026.

Dette dokumentet samler beslutninger som ellers ligger spredt i historiske notater. Det dokumenterer føringer, ikke at funksjonene er ferdig implementert. Se [STATUS](STATUS.md) for levering og [TODO](TODO.md) for arbeidet som gjenstår.

## Samlet game bible og UFO-feltundersøkelser, 6. november

Tom bestiller alt spilldesign, gameplay og Story samlet i [GAME BIBLE](../Design/GAME_BIBLE.md) på GitHub. Hovedbibelen er samlet designfasit; gamle detaljplaner, originale kildepakker og produksjons-JSON bevares som sporbare vedlegg. Active beholder egne roller for status, oppgaver og vedtak.

Tom vil kunne følge UFO-observasjoner og nærkontakter utenfor SARO på tvers av motellet, byen, gården og andre spillbare steder: observere, fotografere, finne gjenstander og undersøke. Hendelser og etterspor bygger på eksisterende saksmotor, Director og fotoløkken. SARO er fortsatt arbeidssted og sted for analyse. Dette utvider D-23/D-25s første avgrensning til basen og byens forsyningsrolle; det opphever ikke Story-kanon.

Gården skal også gi skrekk: mange mystiske lys sett fra SARO, tur ut, søkbart forlatt gårdshus, mystiske ulyder, lett tåke og ubehagelig atmosfære. Noe rart kan skje, med mulig flukt eller bortføring og tapt tid i Freeform. Dette overstyrer eldre avgrensning mot tidstap for akkurat det nye Freeform-forløpet; Storys tilfeldige hendelser får fortsatt ikke hoppe over faste tidspunkt. Huset er ny designbestilling, ikke ferdig interiør; kuer og eksisterende gårdsadkomst bevares. Detaljert varighet, plassering og balansering er åpne i bibelen. Sikret arbeid og en praktisk vei tilbake skal bevares.

## Freeform først, 6. november 2026

Tom bestemte at alt til Freeform fullføres først. Story fortsetter når han er fornøyd med Freeform, fordi all mekanikk og grafikk da er ferdig. Eksisterende Story skal fortsatt bestå selvtestene, men Story-punkter venter med mindre Tom ber om dem.

Tom 7. november: Freeform går foran Story, og Freeform skal brukes til Story-elementer. Story tilpasses Freeform senere, så endringer i modeller, terreng og oppførsel skjermes ikke for Story.

## Toms avgjørelser etter kritikerrundene, 7. november 2026

Bygg: gårdshusets gamle boks og SARO utvendig må bli troverdige, og kontrollrommet mangler følelsen av et sted der folk har levd. Jeepen byttes til en generisk bil. **Sekskantgulvet i SARO byttes;** dette går foran `SARO_1986.md`. Spill: Tom er enig i kritikerens funn om hva som løfter spillet: dag til forberedelse, kveld til jobb, natt til fare, grunn til å dra ut og retning i uke to. Noe kan komme før kl. 01, så kl. 02 er ikke lenger en fast grense, men kvelden skal fortsatt være vanlig jobb. Tid skal ha en pris, og bortføring skal koste. Fremgang, godkjent av Tom: peiling fra antennene, nye steder, CB-radio og 1420 AM, lyskaster på bilen og magnetometer først; deretter feltlogger nr. 2, bedre film og kamera, sikkerhetslys og alarm på SARO, tillit i Caliche og feltseng og motellrom. Director og editor, godkjent av Tom: ett regelsett og en datafil, ikke omskriving, og en lokal nettside som editor, med rytmen først og saker etterpå. Ro etter fare gjelder alle systemer. Høyst ett farlig møte per natt: dør, stedsbesøk, `service_presence` og bortføring teller, ikke titting og lys. Datafilen overstyres i installert spill bare med egen startbryter. Story-filer er bare til lesing til Freeform er ferdig. Mer skal skje i løpet av en natt, og F10 skal kunne teste alt. Bakkens farger beholdes som installert i `d150d75d`, og fjellene får farger fra flyfoto.

Story: Tom 7. november: «Flat verden i Story utgår.» Story går over til samme verden som Freeform (relieff, fjell, veikant, gårdsbygg, bakke). Tom 7. november, retning for Story: enklere og med mer hvorfor. Kjernen er at Reyes ringer etter hjelp og Ortiz blir tatt, fordi han ringte. Story går over 3-5 kvelder og netter og ender i den faste finalen 14. april. Se [forslaget](../Design/Proposals/STORY_ENKLERE_2026-11-07.md), som går foran scenerekkefølgen og «én natt» i bibelen. Figur per jakt, åpent land under jakt og Earl er ikke avgjort.

Freeforms motivasjon, Tom 7. november: «Forrige nattvakt: Reyes fra Story mode.» Freeform foregår etter Story: Reyes og Ortiz er borte, og spilleren er ny nattevakt. Røde tråd er hva som skjedde med forgjengeren. Forslag fra Claude, gjelder til Tom sier annet: start mandag 28. april 1986; spilleren er ikke Reyes; Freeform viser bare utsiden (at han var nattevakt, forsvant 14. april, at Ortiz også er borte, at originalene er borte), aldri 47 sekunder, 1947-bildet, C eller den tredje silhuetten. SARO skal legges ned med mindre stasjonen virker; vurdering etter ca. 30 dager med ulike utfall, og spilleren kan dra og få egen slutt. Bibelens setning om at spilleren er Reyes gjelder bare Story.

Dale, Tom 7. november: Dale er en sint tysker, og temperamentet er inspirert av Klaus Kinski. Han er en oppdiktet kontaktperson via telefon og walkie-talkie og møtes aldri fysisk. Temperamentet er inspirasjon: ikke navn, ekte sitater eller etterligning av den virkelige personen.

## Skrekk og bortføring over hele verden, 6. november 2026

Tom vil at skrekk og bortføring i Freeform også kan skje andre steder enn gården, for eksempel kirken, kinoen, fôrbutikken og bensinstasjonen. Inngangen er at spilleren ser rare lys eller får beskjed på walkie-talkie. Lys, spor, reaksjon og bortføring bygges derfor som ett gjenbrukbart system med gården som første sted. [Referanser til gårdshuset](../Design/Concepts/Farmhouse/README.md).

## Opptakslyd på slitt stasjon, 6. november 2026

Tom valgte at opptakslyd skal virke med redusert målehastighet. Slitasje gir en tidskostnad, men stopper ikke lydarbeidet. Originalen lagres i signaltid og spilles av i normal hastighet; under et saktere opptak høres vanlig mottakerlyd.

## Nattskift 4. til 5. november 2026

Tom bestiller arbeid på TODO så langt tiden rekker, med agenter og eksisterende harness, frem til 5. november kl. 08:00 Europe/Oslo. Gameplay-løkken beholder førsteprioritet. Fra 07:30 prioriteres sikring og nødvendige leveringskontroller. Én tung jobb om gangen; den samlede nattstartkontrollen av arbeidskopier og disk gjenbrukes gjennom skiftet. Ingen offentlig spillutgave er bestilt.

## Freeform-arrayets kapasitetsvirkning, 4. november 2026

Som implementeringsvalg for FF-ARRAY-CONSEQUENCES-01 bruker gyldige arraymålinger tilgjengelige antennepar relativt til fullt anlegg: `N*(N-1)/(27*26)`. Eksisterende minimum på 24 antenner beholdes. Det gir 100 % ved 27, omtrent 93 % ved 26 og 79 % ved 24; under 24 stanser opptak og arraykontroll. Dette er spillbalanse, ikke en full radiometersimulering. Trackingmotorer, lokal servicetest, intern åttesekunders referanse og verdensklokke beholder egne tider. Carrier lock viser fortsatt tuning, mens måleraten viser kapasitet; eksisterende lagret kvalitetsfaktor beholdes. Reparasjon gjenoppretter kapasitet fra faktisk antennetilstand, uten en ny lagringsvariabel. Toms vurdering av spillfølelse gjenstår.

## Adaptiv musikk og tydelig lyddesign, 3. november 2026

Tom ønsker VHS-lyd på startskjermen, mer urovekkende klang for skumle signaler og et iMUSE-lignende musikksystem som følger ro, uro og action. Morbidiums eksisterende musikkdirigent er gjenbruksgrunnlag for lydklokke og overgangsprinsipper, tilpasset Unreal og SIGNALs opptak. Viktige lytteøyeblikk og stillhet beholdes. Modem er fortsatt forbeholdt PC/dataoverføringer. Kontrollerte musikkfraser, tempo og separate instrumentlag må skilles fra frie overganger mellom dagens ferdige spor.

## Lydmotor og modem, 3. november 2026

Tom bestiller en lydmotor som kombinerer eksisterende lydopptak i lag og bearbeider dem til nye lyder. Modemlyder er reservert for PC-overføringer og lignende datakommunikasjon. De brukes ikke som array-, radio- eller VHS-forstyrrelser. Arrayets etablerte fire/sju-mønster beholdes. Oppskrifter og motor kan være offentlige; lokale referanseopptak holdes utenfor Git og offentlig distribusjon. Toms oppfølgende bestilling 3. november autoriserer eksplisitt klargjøring, pakking og installasjon lokalt med NASA, Nebula og de øvrige lydkildene; `--stage-runtime` er den bevisste inngangen. Se [lydverkstedet](../../unreal/Signal47/Tools/sound-workshop/README.md).

## Gameplay-løkken er førsteprioritet, 3. november 2026

Tom bestemte kl. 10 at gameplay og gameplay-løkken går foran grafikk og annet arbeid. Claude og Codex samarbeider om en engasjerende løkke bygget på gameplaygjennomgangen (GP-01 til GP-20) og sammenligningen med Signal Simulator, Voices of the Void og Greyhill Incident. Retningen er [saksløkken i Freeform](../Design/Gameplay/FREEFORM_CASE_LOOP.md): rutine betaler driften, mens spilleren tester forklaringer på avvik med fysiske forsøk; en konklusjon godtas bare når forsøkene støtter den. Arbeidsdeling føres i [OPPGAVER](../../unreal/OPPGAVER.md). Kritiske krasj- og verdensfeil beholder sin plass når de blokkerer spilling.

## Nattskift 2. til 3. november 2026

Tom bestiller arbeid med agenter og eksisterende harness frem til 3. november kl. 08:00 Europe/Oslo. Jeep ferdigstilles først; Codex fortsetter nå og samordner med Claude når hans nye tilgjengelighet begynner kl. 02:10. Dette er ikke en ferdigfrist for Claude. Etter Jeep skal Claude også hjelpe med prioriterte TODO-oppgaver og bruke agenter dersom kapasiteten tillater det. Fra kl. 07:30 prioriteres sikring, nødvendige leveringskontroller og morgenstatus. Den samlede ryddingen 2. november kl. 21:32 gjenbrukes gjennom skiftet. Bestillingen gjelder dette nattskiftet; ingen offentlig spillutgave er bestilt.

## UFO-regi og motorvei, 2. november 2026

Tom bestilte implementering med flere agenter og agent-harness ut fra fire visuelle referanser. Retningen er små røde/hvite lys over mørkt landskap, lys på fysiske flater, lav jakt langs veien, et avgrenset hvitt nærpass og en mørk finale med varm underside. Eksisterende modeller, vei og lagringer gjenbrukes; referansebildene er visuell veiledning.

Motorveihendelsen lar spilleren følge, stoppe eller kjøre videre. En egen følgevariant gir faktisk flukt. Maras forsvinning og spillerens observasjon av den er separate fakta, slik at bortvendt kamera ikke fryser historien eller gir en oppdiktet vitneforklaring. Spillerens/bilens eksisterende 47-sekundersregler beholdes. Fotodokumentasjon bruker eksisterende originalkrav og engangsregistrering. Dette er lokal spillimplementering; offentlig utgivelse er ikke bestilt.

## Gameplay og dokumentasjonsfoto, 2. november 2026

Tom har godkjent implementeringsplanen for SARO, Freeform og TAB-dagboken: seks tydelige hovedfaner, frivillige hint i tre nivåer, riktig neste handling på tvers av apparater, konsistente målinger og bevaring av eksisterende saker. Flere agenter implementerer og kryssvurderer; ekte tastatur-/mus-beta skal skilles fra utviklerfixturer.

Senere samme økt bestilte Tom fotografering av ukjente fenomener som del av inntektsløkken. Spillet handler om å undersøke stjerner, signaler og møter med det ukjente. Standard i denne implementeringen: ett tydelig, faktisk rendret og bevart fotografi kan registreres fysisk på OPS for $20 dokumentasjonskreditt per hendelse; samme hendelse kan ikke betale gjentatte ganger. Betaling beviser ikke objektets opprinnelse. Ordinær signaljobb gir fortsatt $20. Nye fenomener gjenbruker faktiske native aktører; tekst alene skal ikke representere en usett ku-abduksjon eller simulert sabotasje.

Toms eksempler omfatter UFO på himmelen, ku-abduksjon, skikkelser ved trafo og array, samt byobservasjoner varslet av sheriff eller radio. Skillet mellom implementerte hendelser og videre forslag føres i [TODO](TODO.md) og [status](STATUS.md). Ingen gjenstart av tidligere stoppet TODO-automasjon eller offentlig utgivelse følger av bestillingen.

## Nattskift 1. til 2. november 2026

Tom bestilte1. november arbeid fra kl.22:00 til kl.08:00 neste morgen, Europe/Oslo, for å gjennomføre mest mulig av TODO. Prioriter felles fysisk PC-betjening og lysstyring, deretter tydelig utstyrsrespons, etterprøvbare arbeidsrelaterte hendelser og skrekk som reagerer på spillerens handlinger. Se [nattprioriteringene](TODO.md#nattskift-1-november-kl2200-til-2-november-kl0800). Engangsbestillingen er avsluttet 2. november ved fristen kl. 08:00; trådautomasjonen `signal-47-nattskift-22-08` er slettet. Dette var ett nattskift, ikke en stående daglig bestilling. Vanlige prosjektregler om kildevern, reduserte tester, én tung jobb og samlet opprydding gjelder.

## Freeform-verdensprøve 1. november

Tom bestiller Survey Station01 åpen og tilgjengelig i Freeform, med kontroll av opplevd grafisk regresjon og ruten til/fra stasjonen. Alderete-avkjøringen ved SARO skal brukes som gårdsadkomst og åpnes; kuene knyttes til gården nærmere SARO, på høyre side etter motellet slik Tom angir. Dette overstyrer eldre avgrensning av gården som stengt bakgrunnsdekor. Ved motellet fjernes skiltet nærmest veien og det gjenværende får mer neon. De fortsatt rapporterte kontorveggene og brå verdensovergangene er åpne P1-funn. [Oppgaver og originalbilder](TODO.md#nye-freeform-spillfunn-1-november-survey-station-rute-motell-og-gård).

## Pakkehåndtering og gjenbruk, oppfølging 1. november

Tom ønsker enkel billasting: Hold en pakkbar gjenstand og bruk egen parkerte bil for å legge den i lasterommet. Lasting prioriteres foran kjøring når hendene er opptatt, med tydelig kvittering og bevart cargo-tilstand. Den fremste gjennomtrengelige av to gjerderekker langs veien fjernes. Den gamle telefonkioskmodellen ved bensinstasjonen erstattes ved å gjenbruke den nyere modellen på andre siden av gaten. Se TRUCK-01, WORLD-03 og PHONE-01 i [TODO](TODO.md#nye-freeform-spillfunn-1-november-survey-station-rute-motell-og-gård).

## Prosjekt og kanon

| ID | Beslutning | Grunnlag |
| --- | --- | --- |
| D-01 | Videre spillarbeid gjelder Unreal-utgaven. Web og Unity bevares som historikk og referanse. | Toms beslutning 22. oktober, [rotens AGENTS](../../AGENTS.md) og [Unreal-reglene](../../unreal/AGENTS.md). |
| D-02 | Spillet er kosmisk etterforskning ved SARO i New Mexico, 1986. Bevar 1420.405 MHz, 47 sekunder, 27 antenner og azimut 026. Spilltekst er engelsk, prosjektdokumentasjon norsk. | Gjeldende AGENTS og historisk kanonreferanse. |
| D-03 | Faktisk kode og siste relevante test-/leveringskvittering avgjør hva som er implementert. Designbilder, manus og en main-merge er separate fra pakket, installert og publisert spill. | Gjeldende arbeidsregler og [STATUS](STATUS.md). |
| D-04 | Lisensierte MetaHuman-/Fab-kilder og avledede private assets holdes utenfor offentlig Git. Bevar egne redigerbare kilder, proveniens og kreditering. | [Unreal-reglene](../../unreal/AGENTS.md) og de enkelte assetrapportene. |

## Historie: hvilke planer styrer?

Nyere eksplisitte beslutninger går foran eldre planer for den delen de endrer. De erstatter ikke automatisk alle andre krav.

1. **Toms siste direkte instrukser** gjelder først. Samlet spilldesign og det avklarte forløpet leses nå i [GAME BIBLE](../Design/GAME_BIBLE.md). Den godkjente [tidslinjen fra 29. oktober](../Design/Story/TIDSLINJE_SC01_SC13_2026-10-29.md) og [produksjonsmanuset fra 30. oktober](../Design/Story/PRODUKSJON_SC01_SC13.md) overstyrer de eldre tidene nedenfor. Punkt 2 til 5 er bevarte detaljkilders innbyrdes rekkefølge, ikke separate designfasiter.
2. **[Ny kveldsrekkefølge, 28. oktober](../Design/Story/KVELD_PLAN_2026-10-28.md)** styrer den endrede forbindelsen mellom SC01, signal/strøm, trafotur og SC02/SC03. Den senere mørke trafoturen og enklere SC01 overstyrer eldre rekkefølge der de er uenige.
3. **[SC02 til SC13-revisjonen](../Design/Story/HISTORIE_SC02_SC13_REVISJON.md)** styrer revidert dramaturgi og fire aktleveranser der den avviker fra den eldre planen. Ny talende sjåfør i SC12 og diktering av måletabeller er kuttet; H08/H09 er slått sammen. Dette er ikke en beskjed om å fjerne den eksisterende bakgrunnssjåføren i diner.
4. **[Implementeringsplanen](../Design/Story/HISTORIE_IMPLEMENTERINGSPLAN.md)** fører scenario-/verdenskontrakter og produksjonsgrunnlag under revisjonene ovenfor. Den gamle regelen om full separat levering per scenario er erstattet av fire sammenhengende akter og det reduserte testregimet.
5. **[MISSION_FLOW](../Design/Story/MISSION_FLOW_1800_ROSWELL.md)** og eldre detaljvedlegg brukes for kontrollverdier, bevis og delscener der nyere beslutninger ikke sier noe annet.

De engelske forfatterkildene i [Narrative/HorrorStory](../../unreal/Narrative/HorrorStory/README.md) er produksjonsgrunnlag. En skrevet scene er ikke en ferdig verdenshandling. Gamle lagrede saker skal ikke få retroaktive krav fra nye innholdsrevisjoner.

## Nye føringer fra spillprøven

Tom bestemte 31. oktober at **0.1, liten Roswell-alien**, kommer først: referansebilder, faktisk Tripo3D-modell, Blender-klargjøring, Unreal-import og bruk i spillets Glimpse-møter. Dette overstyrer arbeidsrekkefølgen, men lukker eller nedgraderer ikke de åpne krasj-/progresjonsfeilene. Den store jakt-alienen beholdes.

| ID | Beslutning | Konsekvens |
| --- | --- | --- |
| D-05 | Signalet rundt 00:13 skal utløse strømsvikten; trafoturen skjer senere i mørket, og første egentlige SC02-møte flyttes mot kl. 02. | Kvelds-/strøm-/reisekoordinatoren og relevante tekster må samordnes. Dette er pågående arbeid, ikke ferdig levering. |
| D-06 | SC01 skal bruke klask mot ruta og avtrykk, med enklere lampekontroll. | Ikke viderefør den tydelige hånden eller unødvendige turer inn/ut som fremtidig fasit. |
| D-07 | Rolig kveldsarbeid og uro skal fylle tiden før midnatt; nye hovedhendelser må passe den endrede rekkefølgen. | Kveldens oppgaver og SKY01 skal gi sammenheng uten tidlige alienmøter eller ventetid som innhold. |
| D-08 | Godkjent terreng/himmel, eksisterende interaksjoner og personlige lagringer bevares under visuelle forbedringer. Nye konkrete spillerfunn er åpne selv om en eldre leveranse bestod tester. | Følg [feilrapporten](Reports/PLAYTEST_TODO_2026-10-28.md); ikke lukk nye feil med gamle matriser. |
| D-09 | Første personpass er MetaHumans som stille stående/sittende mannekenger på de eksisterende stedene. | Radio-kvinne, røykende mann og diner-sjåfør får poserte statiske erstatninger. Bevar dagens servitrise og alle samhandlings-/tilstedeværelsesregler. [Produksjonskontrakt](../../unreal/Signal47/Tools/metahuman/mannequins-plan.json). |
| D-10 | Nanite utvides etter innhold og faktisk nytte, særlig tette solide miljømodeller. | Ingen global konvertering av vegetasjon, glass eller effekter; bevar RT-fallback og collision. [Kildegjennomgang](../Research/Unreal/UNREAL_SKILLS_2026-10-28.md). |
| D-11 | Den ønskede visuelle retningen er et troverdig retro New Mexico-motell og sammenhengende 1986-farger gjennom dag/natt. | Bevar keramikkmønster, mørkt trepanel, KELVORA-TV og de tre detaljerte bilene. Bare den dårlige blokkbilmodellen skulle bort. Fargeforsøk må passe dagens `M_WebTone`. |
| D-12 | Finalen skal vurderes med en stor, langsom UFO-avsløring bak to skybanker som velter fremover fra hver sin side. | SC13 er anbefalt hovedplassering. Konsepter er regi, ikke implementert effekt. [Finalenotat](../Design/Proposals/FINALE_CLOUD_TANK_UFO.md). |
| D-13 | Dales kontor skal være et personlig, rotete konspirasjonskontor fra 1986. Forbindelsesgangen skal få reell plass og bedre geometri; kontor/gang/dører prioriteres før arkiv/fotolab. | Toms føring 29. oktober. Bruk eide originalmodeller før ny modellering/Tripo, bevar fungerende dokumenter, nøkkel/skuff/dører, lysdrivere og Saved. Ingen kjøp eller automatisk fysisk Dale-scene. Tom godkjente den viste og installerte kontorpiloten fra PR251 30. oktober 2026; denne er visuell referanse for videre romarbeid. |
| D-14 | Tom 30. oktober ber om alle14 eksisterende Tripo-figurer importert og plassert som statiske mannekenger foreløpig. | Bruk eksisterende rolleankre og scenevilkår, bevar originalriggene og fungerende dialog. Tripo-servitrisen er reserve ved kjøkkenet; MetaHuman ved disken beholdes. Ingen ny animasjonsløype bestilles nå. |

| D-15 | SARO-gangen forbedres først, deretter kontrollrommet, med samme metode som visuelt godkjent Dales kontor. | Toms bestilling 30. oktober. Bruk eksisterende eide modeller, egne PBR-overflater og faktisk før/etter i Unreal; bevar fungerende spillankre og lagringer. Tom godkjente begge rommene for nå 30. oktober, for installert PR258. |
| D-16 | Troverdig 1980-tallsdesign er en kjerne i spillet. SARO skal ha gjenkjennelige instrumenter, møbler, telefoner, beholdere og tekstiler, med forskjellige materialer og funksjonelle detaljer. | Toms presisering og fire kontorreferanser 30. oktober. Nytt detaljpass prioriterer RX3 og gjenbrukbare Tripo-armaturer; plakater monteres foran maling og blokklær fjernes eller erstattes. [Analyse og føring](../Design/SARO_1986.md). |
| D-17 | Gjenbruk først ved 3D-arbeid. Søk i eksisterende prosjektassets, Toms Tripo-modeller og eide kilder før ny modellering; bruk egnede bibliotekmodeller når samlet kvalitet og tidsbruk blir bedre. | Toms bestilling 31. oktober etter gjennomgang av arbeidsprinsippet. [Konkret arbeidsflyt](../../unreal/AGENTS.md). Bevar plassering, retning, hengsler, gulv og interaksjoner ved teknisk utskifting; sluttkontroll i Unreal. Verktøyvalg er fleksibelt og innebærer ingen automatisk installasjon, betaling eller publisering av råassets. |
| D-18 | Prøv Toms nye Tripo-kjøretøy: truck ved dineren, politibil i politirollen og jeep som kjørbart SARO-kjøretøy. Den gamle SARO-trucken beholdes på sideparkeringen. | Toms bestilling 31. oktober under spilltest. Prøvebytte, ikke endelig kjøretøyvalg. Identifiser originalmodellene, klargjør i Blender og bevar progresjon, inn-/utstigning og lagring. PLAY-31-14 i [TODO](TODO.md) fører implementeringen. |
| D-19 | Bærbar SARO-walkietalkie på skrivebordet ved start, med pickup og uvanlig skurring nær aktive aliens. Alienmøter skal også ha et tydelig lydsignal eller en musikalsk markering, samordnet med radioen. | Toms bestilling 31. oktober. PLAY-31-20/21 i [TODO](TODO.md) fører implementering. Senere Dale-kontakt er foreløpig et forslag. Bevar eksisterende Rom6-radio og dens historie-/lydrolle. |
| D-20 | Stjernehullet skal være større og tydeligere; signalet klargjør meteorregnet, utgang fra bygget starter det, og nok meteorbaner skal gå bak og skjules av tomrommet. | Toms bestilling 31. oktober. PLAY-31-23 og gjenåpnet FX-01 i [TODO](TODO.md). Denne regien går foran tidligere hendelsesklokke alene og baner som bare unngår hullet. Bevar retning/antennekobling, lagring og fri kamerastyring. |
| D-21 | Dagens sammenhengende spill videreføres som Story med planlagt natt, personer, oppdrag og fast slutt. | Toms føring 31. oktober etter VotV-gjennomgangen. By- og motellreisene beholder sine begrunnede roller i historien. [Modusplan](../Design/Story/SARO_STORY_FREEFORM.md). |
| D-22 | Senere Freeform skal ha vedvarende dager og netter, SARO-arbeid og varierte tilfeldige hendelser, aliens og skrekk, uten Story-finalen. | Tom ba om separat modus. Freeform-grunnlaget er siden levert via PR297. Del SARO og relevante systemer, men gi modusene egne fremdriftsregler og lagringer. [TODO](TODO.md#story-og-freeform). |
| D-23 | Undersøkelsesløkken skal hovedsakelig foregå på SARO-basen, ved antennefeltet og i teknisk område. Første implementering styrker Story-kvelden før 00:00. | Toms bestilling 31. oktober: normalreferanse, avvik, fysisk kontroll, sikret bevis, sammenligning og forståelig neste handling. Gjenbruk Observing-kvelden, bevar den senere historieeskaleringen og gamle lagringer. VOTV-01 i [TODO](TODO.md) fører leveringen. |
| D-24 | SARO-utstyret får faste aktive roller, faktisk antennebevegelse og flere fysiske servicesteg i Story før midnatt. | Tom godkjente gjennomføringsplanen 31. oktober: Story først, S-03-service i flere steg, maskinregistrering og fysisk arkiv. Nabobygget føres til senere Freeform. [Plan](../Design/Story/SARO_STORY_FREEFORM.md#aktive-arbeidsstasjoner-og-antenneservice). |
| D-25 | Byen får en forsynings- og reparasjonsrolle i senere Freeform, med SARO som hovedarbeidssted. | Tom ba 31. oktober om å føre bensinstasjon, brukt/ødelagt utstyr, reservedeler og feeder-vedlikehold inn i planen. Første prøve er liten: sikringer, relé, aggregatdrivstoff, reservelager og enkelt døgnbudsjett. [Plan](../Design/Story/SARO_STORY_FREEFORM.md#byen-som-forsynings--og-reparasjonssted-i-freeform), MODE-03C i TODO. |
| D-26 | Freeform bruker vedvarende døgn, hendelsesregissør og fremgang gjennom mysterier og bedre base. | Tom godkjente gjennomføring: 75 aktive minutter per døgn innen valgt 60–90-minuttersramme, lokal recovery som standard og valgfri død/lasting per ny sak. Ingen våpen; bruk utstyr, dører, skjul og flukt. Fysiske besøk kombineres med tvetydige målefunn. [Gjennomføringsplan](../Design/Story/SARO_STORY_FREEFORM.md#godkjent-gjennomføring-vedvarende-døgn-og-hendelsesregissør). |

Tom utvidet 0.1 etter modellleveransen: Roswell-figurene skal ha nysgjerrig, sky AI med lusking/skjul, gode bevegelser og romlige småsteg/taklyder. Dev-menyen skal styre antall og velge stilleståing, aktiv lusking eller jakt. Dette er bestilt implementering; jakt blir et eksplisitt dev-valg, mens vanlige bakgrunnsmøter beholder den nysgjerrige rollen.

## Linux som distribusjonsmål, 31. oktober

Mottakerens Arch/niri/Nvidia-rapport understreker at Linux-distribusjonen skal beskrive faktiske arkitektur-, ABI-, driver- og øktkrav fremfor å kreve Ubuntu. Utviklingsmaskin og støttet spillermaskin holdes atskilt; kompatibilitet krever konkret grafisk prøve. [Linux-veiledningen](Guides/LINUX_RUNTIME.md) fører dagens krav og begrensninger. Bedre oppstartssjekker prioriteres i PLAY-31-38; Flatpak vurderes i SHIP-04 som tillegg, ikke som allerede vedtatt eller verifisert erstatning.

## Arbeid, testing og levering

- **Én aktiv status og todo:** Oppdater [STATUS](STATUS.md) og [TODO](TODO.md) ved relevante endringer. Nye leveringsnotater skal beskrive én leveranse og lenke hit for dagens status, fremfor å opprette konkurrerende «current»-dokumenter.
- **Arbeidsdeling:** [OPPGAVER](../../unreal/OPPGAVER.md) fører hvem som eier filene og den pågående jobben. `log.md` er append-only historikk, ikke en ekstra statusfasit. Bruk avgrensede subagenter aktivt med tydelige filansvar.
- **Test etter risiko:** Raske relevante kontroller og én målrettet prøve av det som er endret. Dokumentasjon trenger ikke motorbygg. Ikke fullmatrise per liten modell, menyfane eller PR. En nødvendig samlet kontroll før installasjon/offentlig release kjøres én gang; gjenta bare ved nye relevante endringer, feil eller konkret uavklart risiko.
- **Én tung jobb:** Blender, Unreal og pakking kjøres sekvensielt gjennom prosjektets vern. Ingen tester samtidig med Toms spilløkt. Ingen fremmed prosess stoppes for å skaffe testvindu.
- **Vanlig Git-levering:** Spill-PR-er går mot `main`; kontroller riktig head og relevante sjekker før merge og bekreft faktisk resultat. Gitt autorisasjon til vanlig PR/main-merge gir ikke automatisk autorisasjon til en ny offentlig spillrelease.
- **Lokal installasjon:** Bevar personlige Saved, testet payload og kontrollert tilbakekopi. Git-merge, lokal installasjon og offentlig nedlasting føres hver for seg.
- **Lagringsplass:** Behold gjeldende verifiserte testbygg og maksimalt to eldre komplette testbygg. Installerte standardfiler og aktive nødvendige kandidater vernes; kanonisk prune bevarer kilder, assets, Saved, logger og bevis.

Uavklarte eldre notater skal merkes og henvises til gjeldende beslutning. De slettes ikke og blir ikke automatisk gjort til nye oppgaver bare fordi et gammelt avkrysningsfelt står åpent.

## Fysisk betjening og oppgavevariasjon i Freeform, 1. november

Tom ber om at PC-handlinger utføres på selve maskinene i spillverdenen, med arbeidsdeling mellom status, mottaker, tracking og OPS. Lydsammenligning får fysiske kontroller ved telefonens spolebåndutstyr. Dette er aktivt arbeid: klokke, antenner og hendelser fortsetter ved utstyret; pausemenyen holder tiden. Fysiske dokumentoriginaler beholdes ved skriver og arkiv.

Bensin skal kunne kjøpes fra bensinstasjonens faktiske REGULAR-pumpe til den parkerte servicebilen. Levering og betaling følger samme måler; lasting skal ikke gjenta en betaling. Eksisterende aggregatkanne og nødvei beholdes.

Tilfeldig variasjon bygger på eksisterende kontekststyrte hendelsesregissør og en avgrenset dagskortgenerator. Generatoren varierer støttede kilder og faktiske måleinnstillinger, bevarer oppgaven gjennom lagring og lager ingen ubegrensede tekstoppdrag uten tilhørende spillhandling. Vedlikehold følger faktisk utstyrstilstand. Flere hendelsesfamilier og langsiktig balanse føres videre i TODO.

Tripo-bilen, Tom 8. november: git-historikken skrives ikke om. Modellen ble laget på betalt Tripo-plan. Bilen skal ligne mer på en jeep og være mindre kantete, uten varemerker eller kopiert design.

Hendelser hvor som helst, Tom 8. november: gårdens hendelser skal kunne komme ved nesten alle bygg i verden, som landing i ørkenen eller som noen som sitter fast langt ute og ber om hjelp. Stedet trekkes tilfeldig (med frø), ikke fast på gården hver gang.
